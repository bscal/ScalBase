#pragma once

// https://github.com/turanszkij/WickedEngine

#include "Core.h"

#include "DArray.h"
#include "QueueThreaded.h"

struct JobArgs
{
	void* StackMemory;		// stack memory shared within the current group (jobs within a group execute serially)
	u32 JobIndex;			// job index relative to dispatch (like SV_DispatchThreadID in HLSL)
	u32 GroupId;			// group index relative to dispatch (like SV_GroupID in HLSL)
	u32 GroupIndex;			// job index relative to group (like SV_GroupIndex in HLSL)
	bool IsFirstJobInGroup;	// is the current job the first one in the group?
	bool IsLastJobInGroup;	// is the current job the last one in the group?
};

struct Job
{
	JobWorkFunc task;
	void* stack;
	JobHandle* handle;
	u32 GroupId;
	u32 groupJobOffset;
	u32 groupJobEnd;
};

struct JobQueue
{
	#define JOB_QUEUE_SIZE 256
	QueueThreaded<Job, JOB_QUEUE_SIZE> HighPriorityQueue;

	_FORCE_INLINE_ void PushBack(const Job& item)
	{
		bool couldEnqueue = HighPriorityQueue.Enqueue(&item);
		SAssert(couldEnqueue);
	}

	_FORCE_INLINE_ bool PopFront(Job& item)
	{
		bool notEmpty = HighPriorityQueue.Dequeue(&item);
		return notEmpty;
	}
};

// Manages internal state and thread management. Will handle joining and destroying threads
// when finished.
struct JobsInternalState
{
	u32 NumCores;
	u32 NumThreads;
	DArray<zpl_thread> Threads;
	JobQueue* JobQueuePerThread;
	zpl_atomic32 IsAlive;
	zpl_atomic32 NextQueueIndex;

	JobsInternalState()
	{
		NumCores = 0;
		NumThreads = 0;
		JobQueuePerThread = nullptr;
		Threads = {};
		IsAlive = {};
		NextQueueIndex = {};
		zpl_atomic32_store(&IsAlive, 1);

		LogInfo("[ Jobs ] Thread state initialized!");
	}

	~JobsInternalState()
	{
		zpl_atomic32_store(&IsAlive, 0); // indicate that new jobs cannot be started from this point

		for (int i = 0; i < Threads.Count; ++i)
		{
			zpl_semaphore_post(&Threads.At(i)->semaphore, 1);
			zpl_thread_join(Threads.At(i));
			zpl_thread_destroy(Threads.At(i));
		}

		LogInfo("[ Jobs ] Thread state shutdown!");
	}
};

internal JobsInternalState JobsState;

void JobsInitialize(u32 maxThreadCount);

u32 JobsGetThreadCount();

// Defines a state of execution, can be waited on
struct JobHandle
{
	zpl_atomic32 Counter;
};

typedef void(*JobWorkFunc)(JobArgs* args);

// Add a task to execute asynchronously. Any idle thread will execute this.
inline void JobsExecute(JobHandle* handle, JobWorkFunc task, void* stack);

// Divide a task onto multiple jobs and execute in parallel.
//	jobCount	: how many jobs to generate for this task.
//	groupSize	: how many jobs to execute per thread. Jobs inside a group execute serially. It might be worth to increase for small jobs
//	task		: receives a JobArgs as parameter
inline void JobsDispatch(JobHandle* handle, u32 jobCount, u32 groupSize, JobWorkFunc task, void* stack);

// Returns the amount of job groups that will be created for a set number of jobs and group size
inline u32 JobsDispatchGroupCount(u32 jobCount, u32 groupSize);

// Check if any threads are working currently or not
_FORCE_INLINE_ bool
JobHandleIsBusy(const JobHandle* handle)
{
	// Whenever the JobHandle label is greater than zero, it means that there is still work that needs to be done
	return zpl_atomic32_load(&handle->Counter) > 0;
}

// Wait until all threads become idle
// Current thread will become a worker thread, executing jobs
inline void JobHandleWait(const JobHandle* handle);

#ifdef _WIN32
#include <Windows.h>

inline void Win32_InitThread(void* handle, unsigned int threadID)
{
	SAssert(handle);
	// Do Windows-specific thread setup:
	HANDLE winHandle = (HANDLE)handle;

	// Put each thread on to dedicated core:
	DWORD_PTR affinityMask = 1ull << threadID;
	DWORD_PTR affinity_result = SetThreadAffinityMask(winHandle, affinityMask);
	SAssert(affinity_result > 0);

	//// Increase thread priority:
	//BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
	//assert(priority_result != 0);
	// Name the thread:
	WCHAR buffer[31] = {};
	wsprintfW(buffer, L"Job_%d", threadID);
	HRESULT hr = SetThreadDescription(winHandle, buffer);
	SAssert(SUCCEEDED(hr));
}
#endif
#ifdef PLATFORM_LINUX
#include <pthread.h>
#endif

//	Start working on a job queue
//	After the job queue is finished, it can switch to an other queue and steal jobs from there
internal void 
Work(u32 startingQueue)
{
	for (u32 i = 0; i < JobsState.NumThreads; ++i)
	{
		Job job;
		JobQueue* job_queue = &JobsState.JobQueuePerThread[startingQueue % JobsState.NumThreads];
		while (job_queue->PopFront(job))
		{
			SAssert(job.task);

			for (u32 j = job.groupJobOffset; j < job.groupJobEnd; ++j)
			{
				JobArgs args;
				args.GroupId = job.GroupId;
				args.StackMemory = job.stack;
				args.JobIndex = j;
				args.GroupIndex = j - job.groupJobOffset;
				args.IsFirstJobInGroup = (j == job.groupJobOffset);
				args.IsLastJobInGroup = (j == job.groupJobEnd - 1);
				job.task(&args);
			}
			zpl_atomic32_fetch_add(&job.handle->Counter, -1);
		}
		++startingQueue; // go to next queue
	}
}

void JobsInitialize(Arena* arena, u32 maxThreadCount)
{
	if (JobsState.NumThreads > 0)
	{
		SCAL_ERROR("Job internal state already initialized");
		return;
	}

	SAssert(maxThreadCount > 0);

	float startTime = GetTime();

	maxThreadCount = Max(1u, maxThreadCount);

	zpl_affinity affinity;
	zpl_affinity_init(&affinity);

	u32 coreCount = (u32)affinity.core_count;
	u32 threadCount = (u32)affinity.thread_count;

	zpl_affinity_destroy(&affinity);

	if (coreCount == 0)
	{
		SCAL_ERROR("coreCount == 0");
	}

	JobsState.NumCores = coreCount;
	// -2, 1 for main thread, 1 so pc can do other things
	JobsState.NumThreads = ClampValue(threadCount - 2, 1, maxThreadCount);

	JobsState.JobQueuePerThread = ArenaPushArray(arena, JobQueue, JobsState.NumThreads);
    JobsState.Threads.Resize(arena, JobsState.NumThreads);
	SAssert(JobsState.Threads);

	for (u32 threadIdx = 0; threadIdx < JobsState.NumThreads; ++threadIdx)
	{
		zpl_thread* thread = JobsState.Threads.At(threadIdx);
		zpl_thread_init(thread);

		zpl_thread_start_with_stack(thread, [](zpl_thread* thread)
			{
				u32* threadIdx = (u32*)thread->user_data;

				while (zpl_atomic32_load(&JobsState.IsAlive))
				{
					Work(*threadIdx);

					// Wait for more work
					zpl_semaphore_wait(&thread->semaphore);
				}

				return (zpl_isize)0;
			}, & threadIdx, sizeof(u32));

		// Platform thread
#ifdef _WIN32
		Win32_InitThread(thread->win32_handle, threadIdx);
#elif defined(PLATFORM_LINUX)
#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); } while (0)

		int ret;
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		size_t cpusetsize = sizeof(cpuset);

		CPU_SET(threadID, &cpuset);
		ret = pthread_setaffinity_np(worker.native_handle(), cpusetsize, &cpuset);
		if (ret != 0)
			handle_error_en(ret, std::string(" pthread_setaffinity_np[" + std::to_string(threadID) + ']').c_str());

		// Name the thread
		std::string thread_name = "wi::job::" + std::to_string(threadID);
		ret = pthread_setname_np(worker.native_handle(), thread_name.c_str());
		if (ret != 0)
			handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());
#undef handle_error_en
#endif
	}

	double timeEnd = GetTime() - startTime;
	const char* infoStr = TextFormat(
		"[ Jobs ] Initialized in [%.3fms]. Cores: %u, Theads: %d. Created %u threads."
		, timeEnd * 1000.0
		, JobsState.NumCores
		, threadCount
		, JobsState.NumThreads);
	LogInfo(infoStr);
	
	SAssert(JobsState.NumCores > 0);
	SAssert(JobsState.NumThreads > 0);
}

u32 JobsGetThreadCount()
{
	return JobsState.NumThreads;
}

void JobsExecute(JobHandle* handle, JobWorkFunc task, void* stack)
{
	SAssert(handle);
	SAssert(task);

	// Context state is updated:
	zpl_atomic32_fetch_add(&handle->Counter, 1);

	Job job;
	job.handle = handle;
	job.task = task;
	job.stack = stack;
	job.GroupId = 0;
	job.groupJobOffset = 0;
	job.groupJobEnd = 1;

	zpl_i32 idx = zpl_atomic32_fetch_add(&JobsState.NextQueueIndex, 1) % JobsState.NumThreads;
	JobsState.JobQueuePerThread[idx].PushBack(job);
	zpl_semaphore_post(&JobsState.Threads.At(idx)->semaphore, 1);
}

void JobsDispatch(JobHandle* handle, u32 jobCount, u32 groupSize, JobWorkFunc task, void* stack)
{
	SAssert(handle);
	SAssert(task);
	if (jobCount == 0 || groupSize == 0)
	{
		return;
	}

	u32 groupCount = JobsDispatchGroupCount(jobCount, groupSize);

	// Context state is updated:
	zpl_atomic32_fetch_add(&handle->Counter, groupCount);

	Job job;
	job.handle = handle;
	job.task = task;
	job.stack = stack;

	for (u32 GroupId = 0; GroupId < groupCount; ++GroupId)
	{
		// For each group, generate one real job:
		job.GroupId = GroupId;
		job.groupJobOffset = GroupId * groupSize;
		job.groupJobEnd = Min(job.groupJobOffset + groupSize, jobCount);
		
		zpl_i32 idx = zpl_atomic32_fetch_add(&JobsState.NextQueueIndex, 1) % JobsState.NumThreads;
		JobsState.JobQueuePerThread[idx].PushBack(job);
		zpl_semaphore_post(&JobsState.Threads.At(idx)->semaphore, 1);
	}
}

u32 JobsDispatchGroupCount(u32 jobCount, u32 groupSize)
{
	// Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
	return (jobCount + groupSize - 1) / groupSize;
}

void JobHandleWait(const JobHandle* handle)
{
	if (JobHandleIsBusy(handle))
	{
		// Wake any threads that might be sleeping:
		//internal_state.wakeCondition.notify_all();
		for (u32 threadId = 0 ; threadId < JobsState.NumThreads; ++threadId)
			zpl_semaphore_post(&JobsState.Threads.At(threadId)->semaphore, 1);

		// work() will pick up any jobs that are on stand by and execute them on this thread:
		zpl_i32 idx = zpl_atomic32_fetch_add(&JobsState.NextQueueIndex, 1) % JobsState.NumThreads;
		Work(idx);

		while (JobHandleIsBusy(handle))
		{
			// If we are here, then there are still remaining jobs that work() couldn't pick up.
			//	In this case those jobs are not standing by on a queue but currently executing
			//	on other threads, so they cannot be picked up by this thread.
			//	Allow to swap out this thread by OS to not spin endlessly for nothing
			zpl_yield_thread();
		}
	}
}
