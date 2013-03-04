/*

  TNKernel real-time kernel

  Copyright © 2004, 2010 Yuri Tiomkin
  PIC32 version modifications copyright © 2013 Anders Montonen
  All rights reserved.

  ver. 2.6

  Assembler: GCC MIPS

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.
*/

    .set mips32r2
    .set nomips16
    .text
    .align 4

    /* External references */

    .extern  tn_curr_run_task, 4
    .extern  tn_next_task_to_run, 4
    .extern  tn_system_state, 4

    /* Public functions declared in this file */

    .global  tn_switch_context_exit
    .global  tn_switch_context
    .global  tn_cpu_save_sr
    .global  tn_cpu_restore_sr
    .global  tn_start_exe
    .global  tn_chk_irq_disabled
    .global  tn_int_exit
    .global  cs0_int_handler
    .global  tn_inside_int

    .global  ffs_asm

    /* Constants */
    .equ IFS0CLR, 0xBF881034
    .equ context_size, 124
    .equ buildmode, simulator           /* workaround for buggy simulator */

/*----------------------------------------------------------------------------
* Interrups should be disabled here
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_start_exe

tn_start_exe:

    li      $t0, 1                      /* 1 -> TN_SYS_STATE_RUNNING */
    la      $t1, tn_system_state
    sb      $t0, 0($t1)

    lw      $k1, tn_curr_run_task       /* = tn_next_task_to_run */
    j       tn_sw_restore
    lw      $sp, 0($k1)                 /* load new task SP */

    .end tn_start_exe

/*----------------------------------------------------------------------------
* Interrups should be disabled here
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_switch_context_exit

tn_switch_context_exit:
    lw      $k1, tn_curr_run_task       /* = tn_next_task_to_run */
    j       tn_sw_restore
    lw      $sp, 0($k1)                 /* load new task SP */

    .end tn_switch_context_exit

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_switch_context

tn_switch_context:

    /* invoke CS0 interrupt */
    mfc0    $t0, $13                    /* c0_cause */
    ori     $t0, 256
    mtc0    $t0, $13                    /* c0_cause */
.if buildmode != simulator
    jr.hb   $ra
.else
    ehb
    jr   $ra
.endif
    nop

    .end tn_switch_context

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_int_exit

tn_int_exit:

    /* invoke CS0 interrupt */
    mfc0    $t0, $13, 0                    /* c0_cause */
    ori     $t0, 256
    mtc0    $t0, $13, 0                    /* c0_cause */
.if buildmode != simulator
    jr.hb   $ra
.else
    ehb
    jr   $ra
.endif
    nop

    .end tn_int_exit

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent cs0_int_handler

cs0_int_handler:

    mfc0    $k0, $13                    /* c0_cause */
    mfc0    $k1, $14                    /* c0_epc */
    addiu   $sp, $sp, -context_size     /* allocate context on stack */
    sw      $k1, 124($sp)               /* save return address */
    mfc0    $k1, $12                    /* c0_status */

    sw      $k1, 120($sp)

    /* enable nested interrupts */
    ins     $k1, $zero, 1, 15           /* clear IPL, UM, ERL, EXL */
    ext     $k0, $k0, 10, 6             /* extract RIPL */
    ins     $k1, $k0, 10, 6             /* set current IPL */
    mtc0    $k1, $12                    /* c0_status */

    sw      $31, 116($sp)               /* save current task registers */
    sw      $30, 112($sp)
    sw      $28, 108($sp)
    sw      $25, 104($sp)
    sw      $24, 100($sp)
    sw      $23, 96($sp)
    sw      $22, 92($sp)
    sw      $21, 88($sp)
    sw      $20, 84($sp)
    sw      $19, 80($sp)
    sw      $18, 76($sp)
    sw      $17, 72($sp)
    sw      $16, 68($sp)
    sw      $15, 64($sp)
    sw      $14, 60($sp)
    sw      $13, 56($sp)
    sw      $12, 52($sp)
    sw      $11, 48($sp)
    sw      $10, 44($sp)
    sw      $9, 40($sp)
    sw      $8, 36($sp)
    sw      $7, 32($sp)
    sw      $6, 28($sp)
    sw      $5, 24($sp)
    sw      $4, 20($sp)
    sw      $3, 16($sp)
    sw      $2, 12($sp)
    sw      $1, 8($sp)
    mfhi    $t0
    mflo    $t1
    sw      $t0, 4($sp)
    sw      $t1, 0($sp)

    di      $zero                       /* disable interrupts */
    ehb

    la      $t0, tn_curr_run_task
    la      $t2, tn_next_task_to_run
    lw      $t1, 0($t0)
    lw      $t2, 0($t2)
    beq     $t1, $t2, tn_sw_restore
    nop

    sw      $sp, 0($t1)                 /* store sp in preempted task's TCB */
    lw      $sp, 0($t2)                 /* get new task's sp */
    sw      $t2, 0($t0)                 /* tn_curr_run_task = tn_next_task_to_run */

tn_sw_restore:

    mfc0    $k0, $13                    /* c0_cause */
    ins     $k0, $zero, 8, 1            /* clear IP0 */
    mtc0    $k0, $13

    li      $k0, IFS0CLR                /* clear cs0 pending bit */
    ori     $k1, $zero, 2
    sw      $k1, 0($k0)

    lw      $k0, 0($sp)
    lw      $k1, 4($sp)
    mtlo    $k0
    mthi    $k1

    lw      $1, 8($sp)
    lw      $2, 12($sp)
    lw      $3, 16($sp)
    lw      $4, 20($sp)
    lw      $5, 24($sp)
    lw      $6, 28($sp)
    lw      $7, 32($sp)
    lw      $8, 36($sp)
    lw      $9, 40($sp)
    lw      $10, 44($sp)
    lw      $11, 48($sp)
    lw      $12, 52($sp)
    lw      $13, 56($sp)
    lw      $14, 60($sp)
    lw      $15, 64($sp)
    lw      $16, 68($sp)
    lw      $17, 72($sp)
    lw      $18, 76($sp)
    lw      $19, 80($sp)
    lw      $20, 84($sp)
    lw      $21, 88($sp)
    lw      $22, 92($sp)
    lw      $23, 96($sp)
    lw      $24, 100($sp)
    lw      $25, 104($sp)
    lw      $28, 108($sp)
    lw      $30, 112($sp)
    lw      $31, 116($sp)

    lw      $k1, 124($sp)
    lw      $k0, 120($sp)
    mtc0    $k1, $14                    /* c0_epc */
    addiu   $sp, $sp, context_size      /* pop stack frame */
    mtc0    $k0, $12                    /* c0_status */
    eret

    .end cs0_int_handler

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_cpu_save_sr

tn_cpu_save_sr:

    di      $v0                         /* Disable interrupts */
.if buildmode != simulator
    jr.hb   $ra                         /* Return with hazard barrier */
.else
    ehb
    jr   $ra                            /* Return with hazard barrier */
.endif
    nop

    .end tn_cpu_save_sr

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_cpu_restore_sr

tn_cpu_restore_sr:

    mtc0    $a0, $12                    /* c0_status */
.if buildmode != simulator
    jr.hb   $ra                         /* Return with hazard barrier */
.else
    ehb
    jr   $ra                            /* Return with hazard barrier */
.endif
    nop

    .end tn_cpu_restore_sr

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_chk_irq_disabled

tn_chk_irq_disabled:

    mfc0    $v0, $12                    /* c0_status */
    andi    $v0, $v0, 1
    jr      $ra
    xori    $v0, $v0, 1

    .end tn_chk_irq_disabled

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_inside_int

tn_inside_int:

    mfc0    $v0, $12                    /* c0_status */
    ext     $v0, $v0, 10, 6             /* extract IPL */
    sltiu   $v0, $v0, 1                 /* IPL < 1 ? */
    jr      $ra
    xori    $v0, $v0, 1                 /* flip result */

    .end tn_inside_int

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent ffs_asm

ffs_asm:

    subu    $t0, $zero, $a0
    and     $t0, $t0, $a0
    clz     $t0, $t0
    ori     $v0, $zero, 32
    jr      $ra
    subu    $v0, $v0, $t0

    .end ffs_asm

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
