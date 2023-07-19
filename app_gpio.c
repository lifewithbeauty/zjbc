#include "app_gpio.h"

/**
 * @函数描述: gpio相关设备控制初始化
 * @return {*}
 */
void app_gpio_init(void)
{
    led_init();  /* led gpio初始化 */
    beep_init(); /* 蜂鸣器beep gpio初始化 */

    // key_gpio_init(); /* 按键引脚初始化 */

    LED_OFF();
    BEEP_OFF();
}

/**
 * @函数描述: 循环执行工作指示灯任务运行，让LED闪烁 1s跳动一次
 * @return {*}
 */
void app_led_run(void)
{
    static u32 time_count = 0;
    if (millis() - time_count < 1000)
        return;
    time_count = millis();
    LED_TOGGLE();
}

/* 系统启动信号 */
void app_setup_start(void)
{
    u8 i;
    for (i = 0; i < 3; i++) /* 蜂鸣器LED 名叫闪烁 示意系统启动 */
    {
        BEEP_ON();
        LED_ON();
        delay_ms(100);
        BEEP_OFF();
        LED_OFF();
        delay_ms(100);
    }
}

/**
 * @函数描述: key按键运行任务
 * @return {*}
 */
void app_key_run(void)
{
    if (key1_pressing != 0) /* 按键1被按下 */
    {
        if (key1_pressing == 1) /* 短按 */
        {
            key1_pressing = 0;
        }
        else if (key1_pressing == 2) /* 长按 */
        {
        }
    }

    if (key2_pressing != 0) /* 按键2被按下 */
    {
        if (key2_pressing == 1)
        {
            key2_pressing = 0;
        }
        else if (key2_pressing == 2)
        {
        }
    }
}
