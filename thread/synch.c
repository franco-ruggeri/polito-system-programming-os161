/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
        struct semaphore *sem;

        sem = kmalloc(sizeof(*sem));
        if (sem == NULL) {
                return NULL;
        }

        sem->sem_name = kstrdup(name);
        if (sem->sem_name == NULL) {
                kfree(sem);
                return NULL;
        }

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
        sem->sem_count = initial_count;

        return sem;
}

void
sem_destroy(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
        kfree(sem->sem_name);
        kfree(sem);
}

void
P(struct semaphore *sem)
{
        KASSERT(sem != NULL);

        /*
         * May not block in an interrupt handler.
         *
         * For robustness, always check, even if we can actually
         * complete the P without blocking.
         */
        KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
        while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
        }
        KASSERT(sem->sem_count > 0);
        sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

        sem->sem_count++;
        KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock * lock_create(const char *name) {
        struct lock *lock;

        lock = kmalloc(sizeof(*lock));
        if (lock == NULL) {
                return NULL;
        }

        lock->lk_name = kstrdup(name);
        if (lock->lk_name == NULL) {
                kfree(lock);
                return NULL;
        }

        // add stuff here as needed
#if OPT_LOCK
#if LOCK_WITH_SEM
	lock->lk_sem = sem_create(lock->lk_name, 1);
#else
	lock->lk_wchan = wchan_create(lock->lk_name);
#endif
	lock->lk_owner = NULL;
	lock->lk_lock = kmalloc(sizeof(*lock->lk_lock));
	spinlock_init(lock->lk_lock);
#endif

        return lock;
}

void lock_destroy(struct lock *lock) {
        KASSERT(lock != NULL);

        // add stuff here as needed
#if OPT_LOCK
#if LOCK_WITH_SEM
	sem_destroy(lock->lk_sem);
#else
	wchan_destroy(lock->lk_wchan);
#endif
	spinlock_cleanup(lock->lk_lock);
	kfree(lock->lk_lock);
#endif
        kfree(lock->lk_name);
        kfree(lock);
}

void lock_acquire(struct lock *lock) {
        // Write this
#if OPT_LOCK
	KASSERT(lock != NULL);
	KASSERT(!lock_do_i_hold(lock));
#if LOCK_WITH_SEM
	P(lock->lk_sem);
#endif
	spinlock_acquire(lock->lk_lock);
#if !LOCK_WITH_SEM
	while (lock->lk_owner)
		wchan_sleep(lock->lk_wchan, lock->lk_lock);
#endif
	lock->lk_owner = curthread;
	spinlock_release(lock->lk_lock);
#else
        (void)lock;  // suppress warning until code gets written
#endif
}

void lock_release(struct lock *lock) {
        // Write this
#if OPT_LOCK
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));
	spinlock_acquire(lock->lk_lock);
	lock->lk_owner = NULL;
#if !LOCK_WITH_SEM
	wchan_wakeone(lock->lk_wchan, lock->lk_lock);
#endif
	spinlock_release(lock->lk_lock);
#if LOCK_WITH_SEM
	V(lock->lk_sem);
#endif
#else
        (void)lock;  // suppress warning until code gets written
#endif
}

bool lock_do_i_hold(struct lock *lock) {
        // Write this
#if OPT_LOCK
	int result;
	spinlock_acquire(lock->lk_lock);
	result = lock->lk_owner==NULL ? false : lock->lk_owner == curthread;
	spinlock_release(lock->lk_lock);
	return result;
#else
        (void)lock;  // suppress warning until code gets written
        return true; // dummy until code gets written
#endif
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
        struct cv *cv;

        cv = kmalloc(sizeof(*cv));
        if (cv == NULL) {
                return NULL;
        }

        cv->cv_name = kstrdup(name);
        if (cv->cv_name==NULL) {
                kfree(cv);
                return NULL;
        }

        // add stuff here as needed
#if OPT_CV
	cv->cv_wchan = wchan_create(cv->cv_name);
	cv->cv_lock = kmalloc(sizeof(*cv->cv_lock));
	spinlock_init(cv->cv_lock);
#endif

        return cv;
}

void
cv_destroy(struct cv *cv)
{
        KASSERT(cv != NULL);

        // add stuff here as needed
#if OPT_CV
	wchan_destroy(cv->cv_wchan);
	spinlock_cleanup(cv->cv_lock);
	kfree(cv->cv_lock);
#endif

        kfree(cv->cv_name);
        kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
        // Write this
#if OPT_CV
	spinlock_acquire(cv->cv_lock);
	lock_release(lock);
	wchan_sleep(cv->cv_wchan, cv->cv_lock);
	/*
	 * It must be done before lock_acquire() to avoid a spinlock
	 * held by the thread while sleeping, not permitted.
	 * Why not permitted? Other threads can be in busy waiting
	 * on spinlock_acquire() while the thread owning the spinlock
	 * is sleeping, not efficient (spinlock must be used for short waits).
	 * As a consequence, lock acquire upon wakeup is not guaranteed, 
	 * but it is compliant to Mesa semantic (see synch.h).
	 * Due to this, the client has also the responsibility to use while 
	 * instead of if (see slides).
	 */
	spinlock_release(cv->cv_lock);
	lock_acquire(lock);
#else
        (void)cv;    // suppress warning until code gets written
        (void)lock;  // suppress warning until code gets written
#endif
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
        // Write this
#if OPT_CV
	spinlock_acquire(cv->cv_lock);	
	KASSERT(lock_do_i_hold(lock));
	wchan_wakeone(cv->cv_wchan, cv->cv_lock);
	spinlock_release(cv->cv_lock);	
#else
	(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
#endif
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
#if OPT_CV
	spinlock_acquire(cv->cv_lock);	
	KASSERT(lock_do_i_hold(lock));
	wchan_wakeall(cv->cv_wchan, cv->cv_lock);
	spinlock_release(cv->cv_lock);	
#else
	(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
#endif
}
