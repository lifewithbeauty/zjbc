#include "main.h" /* 包含各类驱动文件 */
#include "tcs34725/y_tcs34725.h"
#include "ws2812b/y_ws2812b.h"

// 初始化其他
void others_init(void);
void loop_action(void);
void SWJ_gpio_init(void); /* SWJ引脚配置 */

uint8_t g_start_flag;
uint8_t array[128];

int main(void)
{
    rcc_init();      /* 时钟初始化 */
    SysTick_Init();  /* 初始化系统嘀答定时器，1ms定时一次 */
    SWJ_gpio_init(); /* 禁用(JTAG-DP + SW-DP) */

    servo_init();             /* 舵机初始化 */
    TIM2_init(20000, 72 - 1); /* 初始化定时器2，用于pwm控制舵机 */

    spi_flash_init(); /* 初始化SPI FLASH的IO口 */

    app_gpio_init(); /* 初始化gpio相关引脚 */
    app_uart_init(); /*  初始化相关串口 */
    app_ps2_init();  /* 初始化PS2手柄 */

    app_sensor_init(); /* 初始化传感器功能 */

    others_init(); /* 初始化其他 */

    interrupt_open();  /* 初始化总中断 */
    

    // kinematics 90mm 105mm 98mm 150mm
    // setup_kinematics(90, 105, 98, 150, &kinematics);

    while (1)
    {
        if(start_flag)
        {
            app_setup_start(); /* 应用程序开始 */
            g_start_flag = 1;
            start_flag = 0;
            sprintf((char *)array, "{#000P1500T2000!#001P2000T2000!#002P2000T2000!#003P0900T2000!#004P1500T2000!#005P1500T2000!}");
            uart3_send_str(array);
            memset(array,0,sizeof(array)); 

        }
                
        if(g_start_flag)
        {
            ws2812b_fill(COLOR_GREEN);
            app_led_run();  /* 循环执行工作指示灯 */
            app_uart_run(); /* 串口应用循环运行 */
            app_ps2_run();  /* 循环处理PS2手柄上数据 */
            loop_action();  /* 动作组批量执行 */
            loop_monitor(); // 定时保存一些变量

            app_sensor_run(); // 微信小程序功能
        }
        

        /* 逆运动学测试 */
        // kinematics_move(0, 200, 100, 1000);
        // mdelay(2000);
        // kinematics_move(0, 200, 250, 1000);
        // mdelay(2000);
    }
}

// 初始化其他
void others_init(void)
{
    uint8_t i = 0;

    spiFlahsOn(1);
    w25x_read((u8 *)(&eeprom_info), W25Q64_INFO_ADDR_SAVE_STR, sizeof(eeprom_info)); // 读取全局变量

    if (eeprom_info.version != VERSION)
    {                                  // 判断版本是否是当前版本
        eeprom_info.version = VERSION; // 复制当前版本
        eeprom_info.dj_record_num = 0; // 学习动作组变量赋值0
    }

    if (eeprom_info.dj_bias_pwm[DJ_NUM] != FLAG_VERIFY)
    {
        for (i = 0; i < DJ_NUM; i++)
        {
            eeprom_info.dj_bias_pwm[i] = 0;
        }
        eeprom_info.dj_bias_pwm[DJ_NUM] = FLAG_VERIFY;
    }

    // 将偏差带入初始值
    for (i = 0; i < DJ_NUM; i++)
    {
        set_servo((int)i, 1500 + eeprom_info.dj_bias_pwm[i], 0);
    }
    spiFlahsOn(0);

    // 执行预存命令 {G0000#000P1500T1000!#000P1500T1000!}
    if (eeprom_info.pre_cmd[PRE_CMD_SIZE] == FLAG_VERIFY)
    {
        strcpy((char *)uart_receive_buf, (char *)eeprom_info.pre_cmd);
        if (eeprom_info.pre_cmd[0] == '$')
        {
            parse_cmd(eeprom_info.pre_cmd);
        }
        else
        {
            for (i = 16; i < strlen((char *)uart_receive_buf); i += 15)
            {
                uart_receive_buf[i] = '0';
                uart_receive_buf[i + 1] = '0';
                uart_receive_buf[i + 2] = '0';
                uart_receive_buf[i + 3] = '0';
            }
            parse_action(uart_receive_buf);
        }
    }
}

/* 单片机软件复位 */
void soft_reset(void)
{
    printf("stm32 reset\r\n");
    // 关闭所有中断
    __set_FAULTMASK(1);
    // 复位
    NVIC_SystemReset();
}

/* SWJ引脚配置 */
void SWJ_gpio_init(void)
{
    /**********************
    1.执行端口重映射时,复用功能时钟得使能:RCC_APB2Periph_AFIO

    2.  &1.GPIO_Remap_SWJ_Disable: !< Full SWJ Disabled (JTAG-DP + SW-DP)
         此时PA13|PA14|PA15|PB3|PB4都可作为普通IO用了
       为了保存某些调试端口,GPIO_Remap_SWJ_Disable也可选择为下面两种模式：

        &2.GPIO_Remap_SWJ_JTAGDisable: !< JTAG-DP Disabled and SW-DP Enabled
        此时PA15|PB3|PB4可作为普通IO用了

        &3.GPIO_Remap_SWJ_NoJTRST: !< Full SWJ Enabled (JTAG-DP + SW-DP) but without JTRST
        此时只有PB4可作为普通IO用了
    **********************/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE); // 使能 PA 端口时钟
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);                                               // 使能禁止JTAG和SW-DP
    // GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); // 使能禁止JTAG
}
