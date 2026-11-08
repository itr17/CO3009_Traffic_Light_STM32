#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#define SCH_MAX_TASKS       40
#define NO_TASK_ID          0

#include <stdint.h>
#include <main.h>

typedef struct {
    void   (*pTask)(void);   // con trỏ tới hàm task
    uint32_t Delay;           // số tick còn lại (tương đối)
    uint32_t Period;          // chu kỳ, 0 = one-shot task
    uint8_t  RunMe;           // cờ báo task sẵn sàng chạy
    uint32_t TaskID;
} sTask;

void     SCH_Init(void);
uint32_t SCH_Add_Task(void (*pFunc)(void), uint32_t DELAY, uint32_t PERIOD);
uint8_t  SCH_Delete_Task(uint32_t taskID);
void     SCH_Update(void);          // gọi trong ngắt timer
void     SCH_Dispatch_Tasks(void);  // gọi trong while(1)

#endif /* __SCHEDULER_H__ */
