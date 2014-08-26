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

  /* ver 2.7 */

#ifndef _TH_H_
#define _TH_H_

//--- Types

#ifndef BOOL
#define BOOL      int
#endif

#ifndef TRUE
#define TRUE      1
#endif

#ifndef FALSE
#define FALSE     0
#endif

#ifndef NULL
#define NULL      0
#endif

   //--- The system configuration (change it for your particular project)

#ifndef  TN_CHECK_PARAM
# define TN_CHECK_PARAM       1
#endif

#ifndef  TN_MEAS_PERFORMANCE
# define TN_MEAS_PERFORMANCE  1
#endif

#ifndef  USE_MUTEXES
# define USE_MUTEXES          1
#endif

#ifndef  USE_EVENTS
# define USE_EVENTS           1
#endif

   //--- Port

#include "tn_port.h"


//--- Constants

#define  TN_ST_STATE_NOT_RUN              0
#define  TN_ST_STATE_RUNNING              1


#define  TN_TASK_START_ON_CREATION        1
#define  TN_TASK_TIMER                 0x80
#define  TN_TASK_IDLE                  0x40

#define  TN_ID_TASK              ((int)0x47ABCF69)
#define  TN_ID_SEMAPHORE         ((int)0x6FA173EB)
#define  TN_ID_EVENT             ((int)0x5E224F25)
#define  TN_ID_DATAQUEUE         ((int)0x8C8A6C89)
#define  TN_ID_FSMEMORYPOOL      ((int)0x26B7CE8B)
#define  TN_ID_MUTEX             ((int)0x17129E45)
#define  TN_ID_RENDEZVOUS        ((int)0x74289EBD)


#define  TN_EXIT_AND_DELETE_TASK          1

   //-- Task states

#define  TSK_STATE_RUNNABLE            0x01
#define  TSK_STATE_WAIT                0x04
#define  TSK_STATE_SUSPEND             0x08
#define  TSK_STATE_WAITSUSP            (TSK_STATE_SUSPEND | TSK_STATE_WAIT)
#define  TSK_STATE_DORMANT             0x10

   //--- Waiting

#define  TSK_WAIT_REASON_SLEEP            0x0001
#define  TSK_WAIT_REASON_SEM              0x0002
#define  TSK_WAIT_REASON_EVENT            0x0004
#define  TSK_WAIT_REASON_DQUE_WSEND       0x0008
#define  TSK_WAIT_REASON_DQUE_WRECEIVE    0x0010
#define  TSK_WAIT_REASON_MUTEX_C          0x0020          //-- ver 2.x
#define  TSK_WAIT_REASON_MUTEX_C_BLK      0x0040          //-- ver 2.x
#define  TSK_WAIT_REASON_MUTEX_I          0x0080          //-- ver 2.x
#define  TSK_WAIT_REASON_MUTEX_H          0x0100          //-- ver 2.x
#define  TSK_WAIT_REASON_RENDEZVOUS       0x0200          //-- ver 2.x
#define  TSK_WAIT_REASON_WFIXMEM          0x2000

#define  TN_EVENT_ATTR_SINGLE            1
#define  TN_EVENT_ATTR_MULTI             2
#define  TN_EVENT_ATTR_CLR               4

#define  TN_EVENT_WCOND_OR               8
#define  TN_EVENT_WCOND_AND           0x10

#define  TN_MUTEX_ATTR_CEILING           1
#define  TN_MUTEX_ATTR_INHERIT           2

  //-- Errors

#define  TERR_NO_ERR                     0
#define  TERR_OVERFLOW                 (-1) //-- OOV
#define  TERR_WCONTEXT                 (-2) //-- Wrong context context error
#define  TERR_WSTATE                   (-3) //-- Wrong state   state error
#define  TERR_TIMEOUT                  (-4) //-- Polling failure or timeout
#define  TERR_WRONG_PARAM              (-5)
#define  TERR_UNDERFLOW                (-6)
#define  TERR_OUT_OF_MEM               (-7)
#define  TERR_ILUSE                    (-8) //-- Illegal using
#define  TERR_NOEXS                    (-9) //-- Non-valid or Non-existent object
#define  TERR_DLT                     (-10) //-- Waiting object deleted

#define  NO_TIME_SLICE                   0
#define  MAX_TIME_SLICE             0xFFFE

//-- Circular double-linked list queue - for internal using

typedef struct _CDLL_QUEUE
{
   struct _CDLL_QUEUE * prev;
   struct _CDLL_QUEUE * next;
}CDLL_QUEUE;

//-- Task Control Block --

typedef struct _TN_TCB
{
   unsigned int * task_stk;   //-- Pointer to task's top of stack
   CDLL_QUEUE task_queue;     //-- Queue is used to include task in ready/wait lists
   CDLL_QUEUE timer_queue;    //-- Queue is used to include task in timer(timeout,etc.) list
   CDLL_QUEUE * pwait_queue;  //-- Ptr to object's(semaphor,event,etc.) wait list,
                                      // that task has been included for waiting (ver 2.x)
   CDLL_QUEUE create_queue;   //-- Queue is used to include task in create list only

#ifdef USE_MUTEXES

   CDLL_QUEUE mutex_queue;    //-- List of all mutexes that tack locked  (ver 2.x)

#endif

   unsigned int * stk_start;  //-- Base address of task's stack space
   int   stk_size;            //-- Task's stack size (in sizeof(void*),not bytes)
   void * task_func_addr;     //-- filled on creation  (ver 2.x)
   void * task_func_param;    //-- filled on creation  (ver 2.x)

   int  base_priority;        //-- Task base priority  (ver 2.x)
   int  priority;             //-- Task current priority
   int  id_task;              //-- ID for verification(is it a task or another object?)
                                      // All tasks have the same id_task magic number (ver 2.x)

   int  task_state;           //-- Task state
   int  task_wait_reason;     //-- Reason for waiting
   int  task_wait_rc;         //-- Waiting return code(reason why waiting  finished)
   unsigned long tick_count;  //-- Remaining time until timeout
   int  tslice_count;         //-- Time slice counter

#ifdef  USE_EVENTS

   int  ewait_pattern;        //-- Event wait pattern
   int  ewait_mode;           //-- Event wait mode:  _AND or _OR

#endif

   void * data_elem;          //-- Store data queue entry,if data queue is full

   int  activate_count;       //-- Activation request count - for statistic
   int  wakeup_count;         //-- Wakeup request count - for statistic
   int  suspend_count;        //-- Suspension count - for statistic

// Other implementation specific fields may be added below

}TN_TCB;

//----- Semaphore -----

typedef struct _TN_SEM
{
   CDLL_QUEUE  wait_queue;
   int count;
   int max_count;
   int id_sem;     //-- ID for verification(is it a semaphore or another object?)
                         // All semaphores have the same id_sem magic number (ver 2.x)
}TN_SEM;

//----- Eventflag -----

typedef struct _TN_EVENT
{
   CDLL_QUEUE wait_queue;
   int attr;               //-- Eventflag attribute
   unsigned int pattern;   //-- Initial value of the eventflag bit pattern
   int id_event;           //-- ID for verification(is it a event or another object?)
                                // All events have the same id_event magic number (ver 2.x)
}TN_EVENT;

//----- Data queue -----

typedef struct _TN_DQUE
{
   CDLL_QUEUE  wait_send_list;
   CDLL_QUEUE  wait_receive_list;

   void ** data_fifo;   //-- Array of void* to store data queue entries
   int  num_entries;    //-- Capacity of data_fifo(num entries)
   int  tail_cnt;       //-- Counter to processing data queue's Array of void*
   int  header_cnt;     //-- Counter to processing data queue's Array of void*
   int  id_dque;        //-- ID for verification(is it a data queue or another object?)
                               // All data queues have the same id_dque magic number (ver 2.x)
}TN_DQUE;

//----- Fixed-sized blocks memory pool --------------

typedef struct _TN_FMP
{
   CDLL_QUEUE wait_queue;

   unsigned int block_size; //-- Actual block size (in bytes)
   int num_blocks;          //-- Capacity (Fixed-sized blocks actual max qty)
   void * start_addr;       //-- Memory pool actual start address
   void * free_list;        //-- Ptr to free block list
   int fblkcnt;             //-- Num of free blocks
   int id_fmp;              //-- ID for verification(is it a fixed-sized blocks memory pool or another object?)
                                 // All Fixed-sized blocks memory pool have the same id_fmp magic number (ver 2.x)
}TN_FMP;


//----- Mutex ------------

typedef struct _TN_MUTEX
{
   CDLL_QUEUE wait_queue;        //-- List of tasks that wait a mutex
   CDLL_QUEUE mutex_queue;       //-- To include in task's locked mutexes list (if any)
   CDLL_QUEUE lock_mutex_queue;  //-- To include in system's locked mutexes list
   int attr;                     //-- Mutex creation attr - CEILING or INHERIT

   TN_TCB * holder;              //-- Current mutex owner(task that locked mutex)
   int ceil_priority;            //-- When mutex created with CEILING attr
   int cnt;                      //-- Reserved
   int id_mutex;                 //-- ID for verification(is it a mutex or another object?)
                                       // All mutexes have the same id_mutex magic number (ver 2.x)
}TN_MUTEX;

//-- Thanks to megajohn from electronix.ru - for IAR Embedded C++ compatibility
#ifdef __cplusplus
extern "C"  {
#endif

 //-- Global vars

extern CDLL_QUEUE tn_ready_list[TN_NUM_PRIORITY];   //-- all ready to run(RUNNABLE) tasks
extern CDLL_QUEUE tn_create_queue;                  //-- all created tasks(now - for statictic only)
extern volatile int tn_created_tasks_qty;           //-- num of created tasks
extern CDLL_QUEUE tn_wait_timeout_list;             //-- all tasks that wait timeout expiration


extern volatile int tn_system_state;    //-- System state -(running/not running,etc.)

extern TN_TCB * tn_curr_run_task;       //-- Task that  run now
extern TN_TCB * tn_next_task_to_run;    //-- Task to be run after switch context

extern volatile unsigned int tn_ready_to_run_bmp;
extern volatile unsigned long tn_idle_count;
extern volatile unsigned long tn_curr_performance;

extern volatile int tn_int_nest_count;

extern void * tn_user_sp;               //-- Saved task stack pointer
extern void * tn_int_sp;                //-- Saved ISR stack pointer

 //-- Thanks to Vyacheslav Ovsiyenko - for his highly optimized code

#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field)     \
        ((type *)((unsigned char *)(address) - (unsigned char *)(&((type *)0)->field)))
#endif

//-- v.2.7

#define get_task_by_tsk_queue(que)                  \
        (que ? CONTAINING_RECORD(que, TN_TCB, task_queue) : 0)

#define get_task_by_timer_queque(que)               \
        (que ? CONTAINING_RECORD(que, TN_TCB, timer_queue) : 0)

#define get_mutex_by_mutex_queque(que)              \
        (que ? CONTAINING_RECORD(que, TN_MUTEX, mutex_queue) : 0)

#define get_mutex_by_wait_queque(que)               \
        (que ? CONTAINING_RECORD(que, TN_MUTEX, wait_queue) : 0)

#define get_task_by_block_queque(que)  \
        (que ? CONTAINING_RECORD(que, TN_TCB, block_queue) : 0)

#define get_mutex_by_lock_mutex_queque(que) \
        (que ? CONTAINING_RECORD(que, TN_MUTEX, mutex_queue) : 0)


   //--- User function

void tn_app_init(void);

//----- tn.c ----------------------------------

void tn_start_system(void);
void tn_tick_int_processing(void);
int tn_sys_tslice_ticks(int priority, int value);

//----- tn_tasks.c ----------------------------------

int tn_task_create(TN_TCB * task,
                 void (*task_func)(void *param),
                 int priority,
                 unsigned int * task_stack_start,
                 int task_stack_size,
                 void * param,
                 int option);
int tn_task_suspend(TN_TCB * task);
int tn_task_resume(TN_TCB * task);
int tn_task_sleep(unsigned long timeout);
int tn_task_wakeup(TN_TCB * task);
int tn_task_iwakeup(TN_TCB * task);
int tn_task_activate(TN_TCB * task);
int tn_task_iactivate(TN_TCB * task);
int tn_task_release_wait(TN_TCB * task);
int tn_task_irelease_wait(TN_TCB * task);
void tn_task_exit(int attr);
int tn_task_terminate(TN_TCB * task);
int tn_task_delete(TN_TCB * task);
int tn_task_change_priority(TN_TCB * task, int new_priority);

  //-- Routines

void task_set_dormant_state(TN_TCB* task);
void task_to_non_runnable(TN_TCB * task);
void task_to_runnable(TN_TCB * task);
int  task_wait_complete(TN_TCB * task);
void task_curr_to_wait_action(CDLL_QUEUE * wait_que,
                              int wait_reason,
                              unsigned long timeout);
int change_running_task_priority(TN_TCB * task, int new_priority);
void set_current_priority(TN_TCB * task, int priority);
void find_next_task_to_run(void);

//----- tn_sem.c ----------------------------------

int tn_sem_create(TN_SEM * sem, int start_value, int max_val);
int tn_sem_delete(TN_SEM * sem);
int tn_sem_signal(TN_SEM * sem);
int tn_sem_isignal(TN_SEM * sem);
int tn_sem_acquire(TN_SEM * sem, unsigned long timeout);
int tn_sem_polling(TN_SEM * sem);
int tn_sem_ipolling(TN_SEM * sem);

//----- tn_dqueue.c ----------------------------------

int tn_queue_create(TN_DQUE * dque,
                    void ** data_fifo,
                    int num_entries);
int tn_queue_delete(TN_DQUE * dque);
int tn_queue_send(TN_DQUE * dque, void * data_ptr, unsigned long timeout);
int tn_queue_send_polling(TN_DQUE * dque, void * data_ptr);
int tn_queue_isend_polling(TN_DQUE * dque, void * data_ptr);
int tn_queue_receive(TN_DQUE * dque, void ** data_ptr, unsigned long timeout);
int tn_queue_receive_polling(TN_DQUE * dque, void ** data_ptr);
int tn_queue_ireceive(TN_DQUE * dque, void ** data_ptr);

//-------- tn_event.c -----------------------------

int tn_event_create(TN_EVENT * evf,
                      int attr,
                      unsigned int pattern);
int tn_event_delete(TN_EVENT * evf);
int tn_event_wait(TN_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern,
                    unsigned long timeout);
int tn_event_wait_polling(TN_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern);
int tn_event_iwait(TN_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern);
int tn_event_set(TN_EVENT * evf, unsigned int pattern);
int tn_event_iset(TN_EVENT * evf, unsigned int pattern);
int tn_event_clear(TN_EVENT * evf, unsigned int pattern);
int tn_event_iclear(TN_EVENT * evf, unsigned int pattern);

//----- tn_mem.c ----------------------------------

int tn_fmem_create(TN_FMP  * fmp,
                     void * start_addr,
                     unsigned int block_size,
                     int num_blocks);
int tn_fmem_delete(TN_FMP * fmp);
int tn_fmem_get(TN_FMP * fmp, void ** p_data, unsigned long timeout);
int tn_fmem_get_polling(TN_FMP * fmp, void ** p_data);
int tn_fmem_get_ipolling(TN_FMP * fmp, void ** p_data);
int tn_fmem_release(TN_FMP * fmp, void * p_data);
int tn_fmem_irelease(TN_FMP * fmp, void * p_data);

  //--- tn_mutex.c ---

int tn_mutex_create(TN_MUTEX * mutex,
                    int attribute,
                    int ceil_priority);
int tn_mutex_delete(TN_MUTEX * mutex);
int tn_mutex_lock(TN_MUTEX * mutex, unsigned long timeout);
int tn_mutex_lock_polling(TN_MUTEX * mutex);
int tn_mutex_unlock(TN_MUTEX * mutex);

        //-- Routines

int find_max_blocked_priority(TN_MUTEX * mutex, int ref_priority);
int try_lock_mutex(TN_TCB * task);
int do_unlock_mutex(TN_MUTEX * mutex);

  //--- tn_port.c ---

unsigned int * tn_stack_init(void * task_func,
                             void * stack_start,
                             void * param);

  //--- tn_user.c ---

void tn_cpu_int_enable(void);
void tn_cpu_int_disable(void);
int  tn_inside_int(void);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
