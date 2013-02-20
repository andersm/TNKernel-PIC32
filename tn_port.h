/*

  TNKernel real-time kernel

  Copyright © 2004, 2010 Yuri Tiomkin
  PIC32 version modifications copyright © 2013 Anders Montonen
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

   /* ver 2.6  */



#ifndef  _TN_PORT_H_
#define  _TN_PORT_H_


  /* --------- PIC32 port -------- */

#if defined (__XC32)

#define align_attr_start
#define align_attr_end     __attribute__((aligned(0x8)))

#else

  #error "Unknown compiler"

#endif

#define  TN_TIMER_STACK_SIZE       128
#define  TN_IDLE_STACK_SIZE        80
#define  TN_MIN_STACK_SIZE         72

#define  TN_BITS_IN_INT            32

#define  TN_ALIG                   sizeof(void*)

#define  MAKE_ALIG(a)  ((sizeof(a) + (TN_ALIG-1)) & (~(TN_ALIG-1)))

#define  TN_PORT_STACK_EXPAND_AT_EXIT  40

  //----------------------------------------------------

#define  TN_NUM_PRIORITY        TN_BITS_IN_INT  //-- 0..31  Priority 0 always is used by timers task

#define  TN_WAIT_INFINITE       0xFFFFFFFF
#define  TN_FILL_STACK_VAL      0xFFFFFFFF
#define  TN_INVALID_VAL         0xFFFFFFFF

    //-- Assembler functions prototypes

#ifdef __cplusplus
extern "C"  {
#endif

  void  tn_switch_context_exit(void);
  void  tn_switch_context(void);

  unsigned int tn_cpu_save_sr(void);
  void  tn_cpu_restore_sr(unsigned int sr);
  void  tn_start_exe(void);

  int   tn_chk_irq_disabled(void);
  int   ffs_asm(unsigned int val);

  void tn_int_enter(void);
  void tn_int_exit(void);
  void tn_pic32_disable_interrupts(void);
  void tn_pic32_enable_interrupts(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

    //-- Interrupt processing   - processor specific

#define  TN_INTSAVE_DATA_INT     int tn_save_status_reg = 0;
#define  TN_INTSAVE_DATA         int tn_save_status_reg = 0;
#define  tn_disable_interrupt()  tn_save_status_reg = tn_cpu_save_sr()
#define  tn_enable_interrupt()   tn_cpu_restore_sr(tn_save_status_reg)

#define  tn_idisable_interrupt() tn_save_status_reg = tn_cpu_save_sr()
#define  tn_ienable_interrupt()  tn_cpu_restore_sr(tn_save_status_reg)

#define  TN_CHECK_INT_CONTEXT   \
             if(!tn_inside_int()) \
                return TERR_WCONTEXT;

#define  TN_CHECK_INT_CONTEXT_NORETVAL  \
             if(!tn_inside_int())     \
                return;

#define  TN_CHECK_NON_INT_CONTEXT   \
             if(tn_inside_int()) \
                return TERR_WCONTEXT;

#define  TN_CHECK_NON_INT_CONTEXT_NORETVAL   \
             if(tn_inside_int()) \
                return ;


#endif
