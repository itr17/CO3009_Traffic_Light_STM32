#ifndef __SCH_PORT_H__
#define __SCH_PORT_H__

#include <stdint.h>
#include "main.h"  /* Dùng main.h của STM32 CubeMX để có đủ thư viện HAL và CMSIS */

/**
 * @brief Khóa ngắt toàn cục và trả về trạng thái ngắt ban đầu.
 * @return uint32_t Giá trị thanh ghi PRIMASK trước khi khóa.
 */
static inline uint32_t SCH_Lock(void) {
    uint32_t primask = __get_PRIMASK(); /* Lưu lại trạng thái ngắt hiện tại */
    __disable_irq();                    /* Tắt ngắt toàn cục */
    return primask;                     /* Trả về trạng thái cũ */
}

/**
 * @brief Khôi phục lại trạng thái ngắt ban đầu.
 * @param primask Giá trị thanh ghi PRIMASK lưu từ SCH_Lock().
 */
static inline void SCH_Unlock(uint32_t primask) {
    __set_PRIMASK(primask);             /* Khôi phục lại trạng thái ngắt trước đó */
}

#endif /* __SCH_PORT_H__ */
