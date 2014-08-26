/*

  TNKernel real-time kernel

  Copyright � 2004, 2013 Yuri Tiomkin
  PIC32 version modifications copyright � 2013 Anders Montonen
  All rights reserved.

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

   /* ver 2.7  */



#ifndef  _TN_PORT_H_
#define  _TN_PORT_H_


  /* --------- PIC32 port -------- */

#if defined (__XC32)

#define align_attr_start
#define align_attr_end     __attribute__((aligned(0x8)))

#else

  #error "Unknown compiler"

#endif

#define  USE_ASM_FFS

#ifndef TN_INT_STACK_SIZE
# define  TN_INT_STACK_SIZE        128  // Size of interrupt stack, in words
#endif

#define  TN_TIMER_STACK_SIZE       68   // Size of timer task stack, in words
#define  TN_IDLE_STACK_SIZE        68   // Size of idle task stack, in words
#define  TN_MIN_STACK_SIZE         68   // Minimum task stack size, in words

#define  TN_BITS_IN_INT            32

#define  TN_ALIG                   sizeof(void*)

#define  MAKE_ALIG(a)  ((sizeof(a) + (TN_ALIG-1)) & (~(TN_ALIG-1)))

#define  TN_PORT_STACK_EXPAND_AT_EXIT  32

  //----------------------------------------------------

#define  TN_NUM_PRIORITY        TN_BITS_IN_INT  //-- 0..31  Priority 0 always is used by timers task

#define  TN_WAIT_INFINITE       0xFFFFFFFF
#define  TN_FILL_STACK_VAL      0xFFFFFFFF
#define  TN_INVALID_VAL         0xFFFFFFFF

#define  ffs_asm(x) (32-__builtin_clz((x)&(0-(x))))

//-- Assembler functions prototypes

#ifdef __cplusplus
extern "C"  {
#endif

  void  tn_switch_context_exit(void);
  void  tn_switch_context(void);

  unsigned int tn_cpu_save_sr(void);
  void  tn_cpu_restore_sr(unsigned int sr);
  void  tn_start_exe(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

    //-- Interrupt processing   - processor specific

#define  TN_INTSAVE_DATA_INT     int tn_save_status_reg = 0;
#define  TN_INTSAVE_DATA         int tn_save_status_reg = 0;

#ifdef __mips16
# define tn_disable_interrupt()  tn_save_status_reg = tn_cpu_save_sr()
# define tn_enable_interrupt()   tn_cpu_restore_sr(tn_save_status_reg)
# define tn_idisable_interrupt() tn_save_status_reg = tn_cpu_save_sr()
# define tn_ienable_interrupt()  tn_cpu_restore_sr(tn_save_status_reg)
#else
# define tn_disable_interrupt()  __asm__ __volatile__("di %0; ehb" : "=d" (tn_save_status_reg))
# define tn_enable_interrupt()   __builtin_mtc0(12, 0, tn_save_status_reg)
# define tn_idisable_interrupt() __asm__ __volatile__("di %0; ehb" : "=d" (tn_save_status_reg))
# define tn_ienable_interrupt()  __builtin_mtc0(12, 0, tn_save_status_reg)
#endif

#define  tn_chk_irq_disabled()   ((__builtin_mfc0(12, 0) & 1) == 0)

#define  TN_CHECK_INT_CONTEXT           \
             if(!tn_inside_int())       \
                return TERR_WCONTEXT;

#define  TN_CHECK_INT_CONTEXT_NORETVAL  \
             if(!tn_inside_int())       \
                return;

#define  TN_CHECK_NON_INT_CONTEXT       \
             if(tn_inside_int())        \
                return TERR_WCONTEXT;

#define  TN_CHECK_NON_INT_CONTEXT_NORETVAL  \
             if(tn_inside_int())            \
                return ;




/*----------------------------------------------------------------------------
 * Interrupt handler wrapper macro for software context saving IPL
 *----------------------------------------------------------------------------*/

#define tn_soft_isr(vec)                                                         \
__attribute__((__noinline__)) void _func##vec(void);                             \
void __attribute__((naked, nomips16))__attribute__((vector(vec))) _isr##vec(void)\
{                                                                                \
   asm volatile(".set mips32r2");                                                \
   asm volatile(".set nomips16");                                                \
   asm volatile(".set noreorder");                                               \
   asm volatile(".set noat");                                                    \
                                                                                 \
   asm volatile("rdpgpr  $sp, $sp");                                             \
                                                                                 \
   /* Increase interrupt nesting count */                                        \
   asm volatile("lui     $k0, %hi(tn_int_nest_count)");                          \
   asm volatile("lw      $k1, %lo(tn_int_nest_count)($k0)");                     \
   asm volatile("addiu   $k1, $k1, 1");                                          \
   asm volatile("sw      $k1, %lo(tn_int_nest_count)($k0)");                     \
   asm volatile("ori     $k0, $zero, 1");                                        \
   asm volatile("bne     $k1, $k0, 1f");                                         \
                                                                                 \
   /* Swap stack pointers if nesting count is one */                             \
   asm volatile("lui     $k0, %hi(tn_user_sp)");                                 \
   asm volatile("sw      $sp, %lo(tn_user_sp)($k0)");                            \
   asm volatile("lui     $k0, %hi(tn_int_sp)");                                  \
   asm volatile("lw      $sp, %lo(tn_int_sp)($k0)");                             \
                                                                                 \
   asm volatile("1:");                                                           \
   /* Save context on stack */                                                   \
   asm volatile("addiu   $sp, $sp, -92");                                        \
   asm volatile("mfc0    $k1, $14");               /* c0_epc*/                   \
   asm volatile("mfc0    $k0, $12, 2");            /* c0_srsctl*/                \
   asm volatile("sw      $k1, 84($sp)");                                         \
   asm volatile("sw      $k0, 80($sp)");                                         \
   asm volatile("mfc0    $k1, $12");               /* c0_status*/                \
   asm volatile("sw      $k1, 88($sp)");                                         \
                                                                                 \
   /* Enable nested interrupts */                                                \
   asm volatile("mfc0    $k0, $13");               /* c0_cause*/                 \
   asm volatile("ins     $k1, $zero, 1, 15");                                    \
   asm volatile("ext     $k0, $k0, 10, 6");                                      \
   asm volatile("ins     $k1, $k0, 10, 6");                                      \
   asm volatile("mtc0    $k1, $12");               /* c0_status*/                \
                                                                                 \
   /* Save caller-save registers on stack */                                     \
   asm volatile("sw      $ra, 76($sp)");                                         \
   asm volatile("sw      $t9, 72($sp)");                                         \
   asm volatile("sw      $t8, 68($sp)");                                         \
   asm volatile("sw      $t7, 64($sp)");                                         \
   asm volatile("sw      $t6, 60($sp)");                                         \
   asm volatile("sw      $t5, 56($sp)");                                         \
   asm volatile("sw      $t4, 52($sp)");                                         \
   asm volatile("sw      $t3, 48($sp)");                                         \
   asm volatile("sw      $t2, 44($sp)");                                         \
   asm volatile("sw      $t1, 40($sp)");                                         \
   asm volatile("sw      $t0, 36($sp)");                                         \
   asm volatile("sw      $a3, 32($sp)");                                         \
   asm volatile("sw      $a2, 28($sp)");                                         \
   asm volatile("sw      $a1, 24($sp)");                                         \
   asm volatile("sw      $a0, 20($sp)");                                         \
   asm volatile("sw      $v1, 16($sp)");                                         \
   asm volatile("sw      $v0, 12($sp)");                                         \
   asm volatile("sw      $at, 8($sp)");                                          \
   asm volatile("mfhi    $v0");                                                  \
   asm volatile("mflo    $v1");                                                  \
   asm volatile("sw      $v0, 4($sp)");                                          \
                                                                                 \
   /* Call ISR */                                                                \
   asm volatile("la      $t0, _func"#vec);                                       \
   asm volatile("jalr    $t0");                                                  \
   asm volatile("sw      $v1, 0($sp)");                                          \
                                                                                 \
   /* Pend context switch if needed */                                           \
   asm volatile("lw      $t0, tn_curr_run_task");                                \
   asm volatile("lw      $t1, tn_next_task_to_run");                             \
   asm volatile("lw      $t0, 0($t0)");                                          \
   asm volatile("lw      $t1, 0($t1)");                                          \
   asm volatile("lui     $t2, %hi(IFS0SET)");                                    \
   asm volatile("beq     $t0, $t1, 1f");                                         \
   asm volatile("ori     $t1, $zero, 2");                                        \
   asm volatile("sw      $t1, %lo(IFS0SET)($t2)");                               \
                                                                                 \
   asm volatile("1:");                                                           \
   /* Restore registers */                                                       \
   asm volatile("lw      $v1, 0($sp)");                                          \
   asm volatile("lw      $v0, 4($sp)");                                          \
   asm volatile("mtlo    $v1");                                                  \
   asm volatile("mthi    $v0");                                                  \
   asm volatile("lw      $at, 8($sp)");                                          \
   asm volatile("lw      $v0, 12($sp)");                                         \
   asm volatile("lw      $v1, 16($sp)");                                         \
   asm volatile("lw      $a0, 20($sp)");                                         \
   asm volatile("lw      $a1, 24($sp)");                                         \
   asm volatile("lw      $a2, 28($sp)");                                         \
   asm volatile("lw      $a3, 32($sp)");                                         \
   asm volatile("lw      $t0, 36($sp)");                                         \
   asm volatile("lw      $t1, 40($sp)");                                         \
   asm volatile("lw      $t2, 44($sp)");                                         \
   asm volatile("lw      $t3, 48($sp)");                                         \
   asm volatile("lw      $t4, 52($sp)");                                         \
   asm volatile("lw      $t5, 56($sp)");                                         \
   asm volatile("lw      $t6, 60($sp)");                                         \
   asm volatile("lw      $t7, 64($sp)");                                         \
   asm volatile("lw      $t8, 68($sp)");                                         \
   asm volatile("lw      $t9, 72($sp)");                                         \
   asm volatile("lw      $ra, 76($sp)");                                         \
                                                                                 \
   asm volatile("di");                                                           \
   asm volatile("ehb");                                                          \
                                                                                 \
   /* Restore context */                                                         \
   asm volatile("lw      $k0, 84($sp)");                                         \
   asm volatile("mtc0    $k0, $14");               /* c0_epc */                  \
   asm volatile("lw      $k0, 80($sp)");                                         \
   asm volatile("mtc0    $k0, $12, 2");            /* c0_srsctl */               \
   asm volatile("addiu   $sp, $sp, 92");                                         \
                                                                                 \
   /* Decrease interrupt nesting count */                                        \
   asm volatile("lui     $k0, %hi(tn_int_nest_count)");                          \
   asm volatile("lw      $k1, %lo(tn_int_nest_count)($k0)");                     \
   asm volatile("addiu   $k1, $k1, -1");                                         \
   asm volatile("sw      $k1, %lo(tn_int_nest_count)($k0)");                     \
   asm volatile("bne     $k1, $zero, 1f");                                       \
   asm volatile("lw      $k1, -4($sp)");                                         \
                                                                                 \
   /* Swap stack pointers if nesting count is zero */                            \
   asm volatile("lui     $k0, %hi(tn_int_sp)");                                  \
   asm volatile("sw      $sp, %lo(tn_int_sp)($k0)");                             \
   asm volatile("lui     $k0, %hi(tn_user_sp)");                                 \
   asm volatile("lw      $sp, %lo(tn_user_sp)($k0)");                            \
                                                                                 \
   asm volatile("1:");                                                           \
   asm volatile("wrpgpr  $sp, $sp");                                             \
   asm volatile("mtc0    $k1, $12");               /* c0_status */               \
   asm volatile("eret");                                                         \
                                                                                 \
   asm volatile(".set reorder");                                                 \
   asm volatile(".set at");                                                      \
                                                                                 \
} __attribute((__noinline__)) void _func##vec(void)




/*----------------------------------------------------------------------------
 * Interrupt handler wrapper macro for shadow register context saving IPL
 *----------------------------------------------------------------------------*/
#define tn_srs_isr(vec)                                                          \
__attribute__((__noinline__)) void _func##vec(void);                             \
void __attribute__((naked, nomips16))__attribute__((vector(vec))) _isr##vec(void)\
{                                                                                \
   asm volatile(".set mips32r2");                                                \
   asm volatile(".set nomips16");                                                \
   asm volatile(".set noreorder");                                               \
   asm volatile(".set noat");                                                    \
                                                                                 \
   asm volatile("rdpgpr  $sp, $sp");                                             \
                                                                                 \
   /* Increase interrupt nesting count */                                        \
   asm volatile("lui     $k0, %hi(tn_int_nest_count)");                          \
   asm volatile("lw      $k1, %lo(tn_int_nest_count)($k0)");                     \
   asm volatile("addiu   $k1, $k1, 1");                                          \
   asm volatile("sw      $k1, %lo(tn_int_nest_count)($k0)");                     \
   asm volatile("ori     $k0, $zero, 1");                                        \
   asm volatile("bne     $k1, $k0, 1f");                                         \
                                                                                 \
   /* Swap stack pointers if nesting count is one */                             \
   asm volatile("lui     $k0, %hi(tn_user_sp)");                                 \
   asm volatile("sw      $sp, %lo(tn_user_sp)($k0)");                            \
   asm volatile("lui     $k0, %hi(tn_int_sp)");                                  \
   asm volatile("lw      $sp, %lo(tn_int_sp)($k0)");                             \
                                                                                 \
   asm volatile("1:");                                                           \
   /* Save context on stack */                                                   \
   asm volatile("addiu   $sp, $sp, -20");                                        \
   asm volatile("mfc0    $k1, $14");               /* c0_epc */                  \
   asm volatile("mfc0    $k0, $12, 2");            /* c0_srsctl */               \
   asm volatile("sw      $k1, 12($sp)");                                         \
   asm volatile("sw      $k0, 8($sp)");                                          \
   asm volatile("mfc0    $k1, $12");               /* c0_status */               \
   asm volatile("sw      $k1, 16($sp)");                                         \
                                                                                 \
   /* Enable nested interrupts */                                                \
   asm volatile("mfc0    $k0, $13");               /* c0_cause */                \
   asm volatile("ins     $k1, $zero, 1, 15");                                    \
   asm volatile("ext     $k0, $k0, 10, 6");                                      \
   asm volatile("ins     $k1, $k0, 10, 6");                                      \
   asm volatile("mtc0    $k1, $12");               /* c0_status */               \
                                                                                 \
   /* Save caller-save registers on stack */                                     \
   asm volatile("mfhi    $v0");                                                  \
   asm volatile("mflo    $v1");                                                  \
   asm volatile("sw      $v0, 4($sp)");                                          \
                                                                                 \
   /* Call ISR */                                                                \
   asm volatile("la      $t0, _func"#vec);                                       \
   asm volatile("jalr    $t0");                                                  \
   asm volatile("sw      $v1, 0($sp)");                                          \
                                                                                 \
   /* Pend context switch if needed */                                           \
   asm volatile("lw      $t0, tn_curr_run_task");                                \
   asm volatile("lw      $t1, tn_next_task_to_run");                             \
   asm volatile("lw      $t0, 0($t0)");                                          \
   asm volatile("lw      $t1, 0($t1)");                                          \
   asm volatile("lui     $t2, %hi(IFS0SET)");                                    \
   asm volatile("beq     $t0, $t1, 1f");                                         \
   asm volatile("ori     $t1, $zero, 2");                                        \
   asm volatile("sw      $t1, %lo(IFS0SET)($t2)");                               \
                                                                                 \
   asm volatile("1:");                                                           \
   /* Restore registers */                                                       \
   asm volatile("lw      $v1, 0($sp)");                                          \
   asm volatile("lw      $v0, 4($sp)");                                          \
   asm volatile("mtlo    $v1");                                                  \
   asm volatile("mthi    $v0");                                                  \
                                                                                 \
   asm volatile("di");                                                           \
   asm volatile("ehb");                                                          \
                                                                                 \
   /* Restore context */                                                         \
   asm volatile("lw      $k0, 12($sp)");                                         \
   asm volatile("mtc0    $k0, $14");               /* c0_epc */                  \
   asm volatile("lw      $k0, 8($sp)");                                          \
   asm volatile("mtc0    $k0, $12, 2");            /* c0_srsctl */               \
   asm volatile("addiu   $sp, $sp, 20");                                         \
                                                                                 \
   /* Decrease interrupt nesting count */                                        \
   asm volatile("lui     $k0, %hi(tn_int_nest_count)");                          \
   asm volatile("lw      $k1, %lo(tn_int_nest_count)($k0)");                     \
   asm volatile("addiu   $k1, $k1, -1");                                         \
   asm volatile("sw      $k1, %lo(tn_int_nest_count)($k0)");                     \
   asm volatile("bne     $k1, $zero, 1f");                                       \
   asm volatile("lw      $k1, -4($sp)");                                         \
                                                                                 \
   /* Swap stack pointers if nesting count is zero */                            \
   asm volatile("lui     $k0, %hi(tn_int_sp)");                                  \
   asm volatile("sw      $sp, %lo(tn_int_sp)($k0)");                             \
   asm volatile("lui     $k0, %hi(tn_user_sp)");                                 \
   asm volatile("lw      $sp, %lo(tn_user_sp)($k0)");                            \
                                                                                 \
   asm volatile("1:");                                                           \
   asm volatile("wrpgpr  $sp, $sp");                                             \
   asm volatile("mtc0    $k1, $12");               /* c0_status */               \
   asm volatile("eret");                                                         \
                                                                                 \
   asm volatile(".set reorder");                                                 \
   asm volatile(".set at");                                                      \
                                                                                 \
} __attribute((__noinline__)) void _func##vec(void)

#endif
