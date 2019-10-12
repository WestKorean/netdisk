待处理的软中断被检查和执行的时机，有三种情况：
    1）从一个硬件中断中返回时，调用irq_exit，该函数会判断，只有当没有中断处理程序运行并且存在被标记的软中断，此时会调用do_softirq函数，运行待处理的软中断处理程序。
    2)在ksoftirqd内核线程中运行。当软中断出现较多时，会唤醒ksoftirqd内核线程，该线程运行（死循环）在比较低的优先级中，执行do_softirq函数。
    3）在那些显示的检查和执行待处理的软中断的代码中，如网络子系统中。
    总结：不管用哪种方式唤起软中断的执行，都是通过调用do_softirq函数来执行的。
asmlinkage void do_softirq(void)
{
 __u32 pending;
 unsigned long flags;
 if (in_interrupt())
  return;
 local_irq_save(flags); //获取需要处理的软中断的位图之前，需要先关中断，避免在得到软中断位图的过程中被中断打断，而恰巧中断处理程序又对软中断进行了标记。
 pending = local_softirq_pending(); //获取待处理的软中断位图，每一位都代表了需要执行的软中断
 if (pending) //如果存在待处理的软中断，才会执行__do_softirq()
  __do_softirq();
 local_irq_restore(flags);
}
#define MAX_SOFTIRQ_RESTART 10

asmlinkage void __do_softirq(void)
{
        // 软件中断处理结构，此结构中包括了 ISR 中
        // 注册的回调函数。
 struct softirq_action *h;
 __u32 pending;
 int max_restart = MAX_SOFTIRQ_RESTART;
 int cpu;

        // 得到当前所有 pending 的软中断。
 pending = local_softirq_pending();
 account_system_vtime(current);

        //
        // 执行到这里要屏蔽其他软中断，这里也就证明了
        // 每个 CPU 上同时运行的软中断只能有一个。
        //
 __local_bh_disable((unsigned long)__builtin_return_address(0));
 trace_softirq_enter();

        //
        // 针对 SMP 得到当前正在处理的 CPU
        //
 cpu = smp_processor_id();
//
// 循环标志
//
restart:
        //
        // 每次循环在允许硬件 ISR 强占前，首先重置软中断
        // 的标志位。
        //
 /* Reset the pending bitmask before enabling irqs */
 set_softirq_pending(0);

        //
        // 到这里才开中断运行，注意：以前运行状态一直是关中断
        // 运行，这时当前处理软中断才可能被硬件中断抢占。也就
        // 是说在进入软中断时不是一开始就会被硬件中断抢占。只有
        // 在这里以后的代码才可能被硬件中断抢占。
        //
 local_irq_enable();

        //
        // 这里要注意，以下代码运行时可以被硬件中断抢占，但
        // 这个硬件 ISR 执行完成后，它的所注册的软中断无法马上运行，
        // 别忘了，现在虽是开硬件中断执行，但前面的 __local_bh_disable()
        // 函数屏蔽了软中断。所以这种环境下只能被硬件中断抢占，但这
        // 个硬中断注册的软中断回调函数无法运行。要问为什么，那是因为
        // __local_bh_disable() 函数设置了一个标志当作互斥量，而这个
        // 标志正是上面的 irq_exit() 和 do_softirq() 函数中的
        // in_interrupt() 函数判断的条件之一，也就是说 in_interrupt() 
        // 函数不仅检测硬中断而且还判断了软中断。所以在这个环境下触发
        // 硬中断时注册的软中断，根本无法重新进入到这个函数中来，只能
        // 是做一个标志，等待下面的重复循环（最大 MAX_SOFTIRQ_RESTART）
        // 才可能处理到这个时候触发的硬件中断所注册的软中断。
        //


        //
        // 得到软中断向量表。
        //
 h = softirq_vec;

        //
        // 循环处理所有 softirq 软中断注册函数。
        // 
 do {
                //
                // 如果对应的软中断设置 pending 标志则表明
                // 需要进一步处理它所注册的函数。
                //
  if (pending & 1) {
                        //
                        // 在这里执行了这个软中断所注册的回调函数。
                        //
   h->action(h);
   rcu_bh_qsctr_inc(cpu);
  }
        //
        // 继续找，直到把软中断向量表中所有 pending 的软
        // 中断处理完成。
        //
  h++;

                //
                // 从代码里可以看出按位操作，表明一次循环只
                // 处理 32 个软中断的回调函数。
                //
  pending >>= 1; 
 } while (pending);

        //
        // 关中断执行以下代码。注意：这里又关中断了，下面的
        // 代码执行过程中硬件中断无法抢占。
        //
 local_irq_disable();

        //
        // 前面提到过，在刚才开硬件中断执行环境时只能被硬件中断
        // 抢占，在这个时候是无法处理软中断的，因为刚才开中
        // 断执行过程中可能多次被硬件中断抢占，每抢占一次就有可
        // 能注册一个软中断，所以要再重新取一次所有的软中断。
        // 以便下面的代码进行处理后跳回到 restart 处重复执行。
        //
 pending = local_softirq_pending();

        //
        // 如果在上面的开中断执行环境中触发了硬件中断，且每个都
        // 注册了一个软中断的话，这个软中断会设置 pending 位，
        // 但在当前一直屏蔽软中断的环境下无法得到执行，前面提
        // 到过，因为 irq_exit() 和 do_softirq() 根本无法进入到
        // 这个处理过程中来。这个在上面详细的记录过了。那么在
        // 这里又有了一个执行的机会。注意：虽然当前环境一直是
        // 处于屏蔽软中断执行的环境中，但在这里又给出了一个执行
        // 刚才在开中断环境过程中触发硬件中断时所注册的软中断的
        // 机会，其实只要理解了软中断机制就会知道，无非是在一些特
        // 定环境下调用 ISR 注册到软中断向量表里的函数而已。
        //

        //
        // 如果刚才触发的硬件中断注册了软中断，并且重复执行次数
        // 没有到 10 次的话，那么则跳转到 restart 标志处重复以上
        // 所介绍的所有步骤：设置软中断标志位，重新开中断执行...
        // 注意：这里是要两个条件都满足的情况下才可能重复以上步骤。 
        //
 if (pending && --max_restart)
  goto restart;

        //
        // 如果以上步骤重复了 10 次后还有 pending 的软中断的话，
        // 那么系统在一定时间内可能达到了一个峰值，为了平衡这点。
        // 系统专门建立了一个 ksoftirqd 线程来处理，这样避免在一
        // 定时间内负荷太大。这个 ksoftirqd 线程本身是一个大循环，
        // 在某些条件下为了不负载过重，它是可以被其他进程抢占的，
        // 但注意，它是显示的调用了 preempt_xxx() 和 schedule()
        // 才会被抢占和切换的。这么做的原因是因为在它一旦调用 
        // local_softirq_pending() 函数检测到有 pending 的软中断
        // 需要处理的时候，则会显示的调用 do_softirq() 来处理软中
        // 断。也就是说，下面代码唤醒的 ksoftirqd 线程有可能会回
        // 到这个函数当中来，尤其是在系统需要响应很多软中断的情况
        // 下，它的调用入口是 do_softirq()，这也就是为什么在 do_softirq()
        // 的入口处也会用 in_interrupt()  函数来判断是否有软中断
        // 正在处理的原因了，目的还是为了防止重入。ksoftirqd 实现
        // 看下面对 ksoftirqd() 函数的分析。
        //
 if (pending)

               //
               // 此函数实际是调用 wake_up_process() 来唤醒 ksoftirqd
               // 
  wakeup_softirqd();

 trace_softirq_exit();

 account_system_vtime(current);

        //
        // 到最后才开软中断执行环境，允许软中断执行。注意：这里
        // 使用的不是 local_bh_enable()，不会再次触发 do_softirq()
        // 的调用。
        // 
 _local_bh_enable();
}
static int ksoftirqd(void * __bind_cpu)
{

        //
        // 显示调用此函数设置当前进程的静态优先级。当然，
        // 这个优先级会随调度器策略而变化。
        //
 set_user_nice(current, 19);

        //
        // 设置当前进程不允许被挂启
        //
 current->flags |= PF_NOFREEZE;

        //
        // 设置当前进程状态为可中断的状态，这种睡眠状
        // 态可响应信号处理等。
        // 
 set_current_state(TASK_INTERRUPTIBLE);

        //
        // 下面是一个大循环，循环判断当前进程是否会停止，
        // 不会则继续判断当前是否有 pending 的软中断需
        // 要处理。
        //
 while (!kthread_should_stop()) {

                //
                // 如果可以进行处理，那么在此处理期间内禁止
                // 当前进程被抢占。
                //
  preempt_disable();

                //
                // 首先判断系统当前没有需要处理的 pending 状态的
                // 软中断
                //
  if (!local_softirq_pending()) {

                        //
                        // 没有的话在主动放弃 CPU 前先要允许抢占，因为
                        // 一直是在不允许抢占状态下执行的代码。
                        //
   preempt_enable_no_resched();

                        //
                        // 显示调用此函数主动放弃 CPU 将当前进程放入睡眠队列，
                        // 并切换新的进程执行（调度器相关不记录在此）
                        //
   schedule();

                        //
                        // 注意：如果当前显示调用 schedule() 函数主动切换的进
                        // 程再次被调度执行的话，那么将从调用这个函数的下一条
                        // 语句开始执行。也就是说，在这里当前进程再次被执行的
                        // 话，将会执行下面的 preempt_disable() 函数。
                        //

                        //
                        // 当进程再度被调度时，在以下处理期间内禁止当前进程
                        // 被抢占。
                        //
   preempt_disable();
  }

                //
                // 设置当前进程为运行状态。注意：已经设置了当前进程不可抢占
                // 在进入循环后，以上两个分支不论走哪个都会执行到这里。一是
                // 进入循环时就有 pending 的软中断需要执行时。二是进入循环时
                // 没有 pending 的软中断，当前进程再次被调度获得 CPU 时继续
                // 执行时。
                //
  __set_current_state(TASK_RUNNING);

                //
                // 循环判断是否有 pending 的软中断，如果有则调用 do_softirq()
                // 来做具体处理。注意：这里又是一个 do_softirq() 的入口点，
                // 那么在 __do_softirq() 当中循环处理 10 次软中断的回调函数
                // 后，如果还有 pending 的话，会又调用到这里。那么在这里则
                // 又会有可能去调用 __do_softirq() 来处理软中断回调函数。在前
                // 面介绍 __do_softirq() 时已经提到过，处理 10 次还处理不完的
                // 话说明系统正处于繁忙状态。根据以上分析，我们可以试想如果在
                // 系统非常繁忙时，这个进程将会与 do_softirq() 相互交替执行，
                // 这时此进程占用 CPU 应该会很高，虽然下面的 cond_resched() 
                // 函数做了一些处理，它在处理完一轮软中断后当前处理进程可能会
                // 因被调度而减少 CPU 负荷，但是在非常繁忙时这个进程仍然有可
                // 能大量占用 CPU。
                //
  while (local_softirq_pending()) {
   /* Preempt disable stops cpu going offline.
      If already offline, we'll be on wrong CPU:
      don't process */
   if (cpu_is_offline((long)__bind_cpu))

                                //
                                // 如果当前被关联的 CPU 无法继续处理则跳转
                                // 到 wait_to_die 标记出，等待结束并退出。
                                // 
    goto wait_to_die;

                        //
                        // 执行 do_softirq() 来处理具体的软中断回调函数。注
                        // 意：如果此时有一个正在处理的软中断的话，则会马上
                        // 返回，还记得前面介绍的 in_interrupt() 函数么。
                        //
   do_softirq();

                        //
                        // 允许当前进程被抢占。
                        //
   preempt_enable_no_resched();
                        
                        //
                        // 这个函数有可能间接的调用 schedule() 来切换当前
                        // 进程，而且上面已经允许当前进程可被抢占。也就是
                        // 说在处理完一轮软中断回调函数时，有可能会切换到
                        // 其他进程。我认为这样做的目的一是为了在某些负载
                        // 超标的情况下不至于让这个进程长时间大量的占用 CPU，
                        // 二是让在有很多软中断需要处理时不至于让其他进程
                        // 得不到响应。
                        //
   cond_resched();

                        //
                        // 禁止当前进程被抢占。
                        //
   preempt_disable();

                        //
                        // 处理完所有软中断了吗？没有的话继续循环以上步骤
                        //
  }

                //
                // 待一切都处理完成后，允许当前进程被抢占，并设置
                // 当前进程状态为可中断状态，继续循环以上所有过程。
                //
  preempt_enable();
  set_current_state(TASK_INTERRUPTIBLE);
 }
   
        //
        // 如果将会停止则设置当前进程为运行状态后直接返回。
        // 调度器会根据优先级来使当前进程运行。
        //
 __set_current_state(TASK_RUNNING);
 return 0;

//
// 一直等待到当前进程被停止
//
wait_to_die:

        //
        // 允许当前进程被抢占。
        //
 preempt_enable();
 /* Wait for kthread_stop */

        //
        // 设置当前进程状态为可中断的状态，这种睡眠状
        // 态可响应信号处理等。
        // 
 set_current_state(TASK_INTERRUPTIBLE);

        //
        // 判断当前进程是否会被停止，如果不是的话
        // 则设置进程状态为可中断状态并放弃当前 CPU
        // 主动切换。也就是说这里将一直等待当前进程
        // 将被停止时候才结束。
        //
 while (!kthread_should_stop()) {
  schedule();
  set_current_state(TASK_INTERRUPTIBLE);
 }

        //
        // 如果将会停止则设置当前进程为运行状态后直接返回。
        // 调度器会根据优先级来使当前进程运行。
        //
 __set_current_state(TASK_RUNNING);
 return 0;
}