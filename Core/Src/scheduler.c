#include <string.h>
#include "scheduler.h"
#include "sch_port.h"

#define SCH_TICK_HALF_RANGE 0x80000000UL

static sTask SCH_tasks_G[SCH_MAX_TASKS];
static uint8_t SCH_count = 0;
static uint32_t newTaskID = 0;
static volatile uint32_t scheduler_ticks = 0;

static uint32_t Get_New_Task_ID(void) {
    if (++newTaskID == NO_TASK_ID) newTaskID++;
    return newTaskID;
}

static uint8_t Is_Due(uint32_t now, uint32_t deadline) {
    return (uint32_t)(now - deadline) < SCH_TICK_HALF_RANGE;
}

static uint8_t Deadline_Before(uint32_t first, uint32_t second) {
    return (uint32_t)(first - second) >= SCH_TICK_HALF_RANGE;
}

static void Insert_Task(sTask task) {
    uint8_t index = 0;
    while (index < SCH_count &&
           !Deadline_Before(task.NextRun, SCH_tasks_G[index].NextRun)) {
        index++;
    }

    for (uint8_t i = SCH_count; i > index; i--)
        SCH_tasks_G[i] = SCH_tasks_G[i - 1U];

    SCH_tasks_G[index] = task;
    SCH_count++;
}

static void Remove_Task_At(uint8_t index) {
    for (uint8_t i = index; i + 1U < SCH_count; i++)
        SCH_tasks_G[i] = SCH_tasks_G[i + 1U];

    SCH_count--;
    memset(&SCH_tasks_G[SCH_count], 0, sizeof(sTask));
}

void SCH_Init(void) {
    memset(SCH_tasks_G, 0, sizeof(SCH_tasks_G));
    SCH_count = 0;
    newTaskID = 0;
    scheduler_ticks = 0;
}

/* O(1): only advance the monotonic tick counter in the timer ISR. */
void SCH_Update(void) {
    scheduler_ticks++;
}

uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD) {
    if (pFunction == 0 ||
        DELAY >= SCH_TICK_HALF_RANGE ||
        PERIOD >= SCH_TICK_HALF_RANGE) {
        return NO_TASK_ID;
    }

    uint32_t m = SCH_Lock();
    if (SCH_count >= SCH_MAX_TASKS) {
        SCH_Unlock(m);
        return NO_TASK_ID;
    }

    uint32_t id = Get_New_Task_ID();
    sTask task;
    task.pTask = pFunction;
    task.NextRun = scheduler_ticks + DELAY;
    task.Period = PERIOD;
    task.RunMe = 0;
    task.TaskID = id;
    Insert_Task(task);
    SCH_Unlock(m);
    return id;
}

void SCH_Dispatch_Tasks(void) {
    for (;;) {
        sTask task;
        uint32_t m = SCH_Lock();
        uint32_t now = scheduler_ticks;

        if (SCH_count == 0 ||
            SCH_tasks_G[0].RunMe ||
            !Is_Due(now, SCH_tasks_G[0].NextRun)) {
            SCH_Unlock(m);
            return;
        }

        task = SCH_tasks_G[0];
        SCH_tasks_G[0].RunMe = 1;
        SCH_Unlock(m);

        task.pTask();

        m = SCH_Lock();
        for (uint8_t i = 0; i < SCH_count; i++) {
            if (SCH_tasks_G[i].TaskID != task.TaskID)
                continue;

            Remove_Task_At(i);
            if (task.Period > 0) {
                now = scheduler_ticks;
                task.NextRun += task.Period;
                uint32_t next_run = task.NextRun;
                if (Is_Due(now, next_run)) {
                    uint32_t missed_periods =
                        (now - next_run) / task.Period + 1U;
                    next_run += missed_periods * task.Period;
                }

                task.NextRun = next_run;
                task.RunMe = 0;
                Insert_Task(task);
            }
            break;
        }
        SCH_Unlock(m);
    }
}

uint8_t SCH_Delete_Task(uint32_t taskID) {
    if (taskID == NO_TASK_ID) return 0;

    uint8_t found = 0;
    uint32_t m = SCH_Lock();
    for (uint8_t i = 0; i < SCH_count; i++) {
        if (SCH_tasks_G[i].TaskID != taskID) continue;
        Remove_Task_At(i);
        found = 1;
        break;
    }
    SCH_Unlock(m);
    return found;
}
