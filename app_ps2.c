#include "app_ps2.h"

#if car_mode
const char *pre_cmd_set_red[PSX_BUTTON_NUM] = {
    // 手柄按键功能字符串 红灯模式下使用
    "<PS2_RED01:#005P0600T2000!^#005PDST!>", // L2
    "<PS2_RED02:#005P2400T2000!^#005PDST!>", // R2
    "<PS2_RED03:#004P0600T2000!^#004PDST!>", // L1
    "<PS2_RED04:#004P2400T2000!^#004PDST!>", // R1
    "<PS2_RED05:#002P2400T2000!^#002PDST!>", // RU
    "<PS2_RED06:#003P0600T2000!^#003PDST!>", // RR
    "<PS2_RED07:#002P0600T2000!^#002PDST!>", // RD
    "<PS2_RED08:#003P2400T2000!^#003PDST!>", // RL
    "<PS2_RED09:$DJR!>",                     // SE
    "<PS2_RED10:>",                          // AL
    "<PS2_RED11:>",                          // AR
    "<PS2_RED12:$DJR!>",                     // ST
    "<PS2_RED13:#001P0600T2000!^#001PDST!>", // LU
    "<PS2_RED14:#000P0600T2000!^#000PDST!>", // LR
    "<PS2_RED15:#001P2400T2000!^#001PDST!>", // LD
    "<PS2_RED16:#000P2400T2000!^#000PDST!>", // LL
};
#endif
const char *pre_cmd_set_grn[PSX_BUTTON_NUM] = {
    // 绿灯模式下按键的配置
    "<PS2_RED01:$DCR:0,18,18,0!^$DCR:0,0,0,0!>",             // L2  左上18
    "<PS2_RED02:$DCR:18,0,0,18!^$DCR:0,0,0,0!>",             // R2	右上18
    "<PS2_RED03:$DCR:0,25,25,0!^$DCR:0,0,0,0!>",           // L1	左上25
    "<PS2_RED04:$DCR:25,0,0,25!^$DCR:0,0,0,0!>",           // R1	右上25
    "<PS2_RED05:$DCR:25,25,25,25!^$DCR:0,0,0,0!>",     // RU	前进25
    "<PS2_RED06:$DCR:25,-25,-25,25!^$DCR:0,0,0,0!>",   // RR	右平移25
    "<PS2_RED07:$DCR:-25,-25,-25,-25!^$DCR:0,0,0,0!>", // RD	后退25
    "<PS2_RED08:$DCR:-25,25,25,-25!^$DCR:0,0,0,0!>",   // RL	左平移25
    "<PS2_RED09:$DJR!>",                                       // SE
    "<PS2_RED10:>",                                            // AL
    "<PS2_RED11:>",                                            // AR
    "<PS2_RED12:$DJR!>",                                       // ST
    "<PS2_RED13:$DCR:18,18,18,18!^$DCR:0,0,0,0!>",         // LU	前进18
    "<PS2_RED14:$DCR:18,-18,18,-18!^$DCR:0,0,0,0!>",       // LR	右转18
    "<PS2_RED15:$DCR:-18,-18,-18,-18!^$DCR:0,0,0,0!>",     // LD	后退18
    "<PS2_RED16:$DCR:-18,18,-18,18!^$DCR:0,0,0,0!>",       // LL	左转18
};
void parse_psx_buf(unsigned char *buf, unsigned char mode);
void loop_ps2_car(void);

/**
 * @函数描述: PS2设备控制初始化
 * @return {*}
 */
void app_ps2_init(void)
{
    ps2_init(); /* PS2引脚初始化 */
}

/**
 * @函数描述: 循环执行工作指示灯任务运行，让LED闪烁 1s跳动一次
 * @return {*}
 */
void app_ps2_run(void)
{
    static unsigned char psx_button_bak[2] = {0};
    static u32 systick_ms_bak = 0;

    // 每50ms处理1次
    if (millis() - systick_ms_bak < 50)
        return;

    systick_ms_bak = millis();

    ps2_write_read(); /* 读取ps2数据 */
//    printf("\r\n 0=%x   1=%x   2=%x   3=%x   4=%x   5=%x  6=%x  7=%x  8=%x", psx_buf[0], psx_buf[1], psx_buf[2], psx_buf[3], psx_buf[4], psx_buf[5], psx_buf[6], psx_buf[7], psx_buf[8]);
//    printf("    ****");
    loop_ps2_car(); /* 处理小车电机摇杆控制 */

    // 对比两次获取的按键值是否相同 ，相同就不处理，不相同则处理
    if ((psx_button_bak[0] == psx_buf[3]) && (psx_button_bak[1] == psx_buf[4]))
    {
    }
    else
    {
        // printf("parse_psx_buf\r\n");
        // 处理buf3和buf4两个字节，这两个字节存储这手柄16个按键的状态
        parse_psx_buf(psx_buf + 3, psx_buf[1]);
        psx_button_bak[0] = psx_buf[3];
        psx_button_bak[1] = psx_buf[4];
    }
}

// 处理麦轮小车电机摇杆控制
void loop_ps2_car(void)
{
    if (car_mode)
    {
        static int car_left1, car_right1, car_left2, car_right2;
        static int car_left1_bak = 0, car_right1_bak = 0, car_left2_bak = 0, car_right2_bak = 0;

        static u8 pss_ly = 0, pss_lx = 0, pss_ry = 0, pss_rx = 0;
        static float gear_dw = 0.8;
        static u8 num = 10;

        if (psx_buf[1] != PS2_LED_RED)
            return;

        //	sprintf((char*)cmd_return, "%d  ,%d  ,%d  ,%d  ", pss_lx,pss_ly,pss_rx,pss_ry);
        //	uart1_send_str(cmd_return);
        //	uart1_send_str((u8 *)"\r\n");
        if (abs_int(127 - psx_buf[7]) < num)
            psx_buf[7] = 127; // 数据较小时不做处理，容易误判，或者小数据干扰
        if (abs_int(127 - psx_buf[8]) < num)
            psx_buf[8] = 127;
        if (abs_int(127 - psx_buf[5]) < num)
            psx_buf[5] = 127; // 数据较小时不做处理，容易误判，或者小数据干扰
        if (abs_int(127 - psx_buf[6]) < num)
            psx_buf[6] = 127;

        pss_lx = psx_buf[7];
        pss_ly = psx_buf[8];
        pss_rx = psx_buf[5];
        pss_ry = psx_buf[6];
        car_left1 = ((-(127 - pss_rx) + (127 - pss_ry) - (127 - pss_lx) + (127 - pss_ly)) / 4) * gear_dw;  // 前左轮速度
        car_right1 = ((+(127 - pss_rx) + (127 - pss_ry) + (127 - pss_lx) + (127 - pss_ly)) / 4) * gear_dw; // 前右轮速度
        car_left2 = ((-(127 - pss_rx) + (127 - pss_ry) + (127 - pss_lx) + (127 - pss_ly)) / 4) * gear_dw;  // 后左轮速度
        car_right2 = ((+(127 - pss_rx) + (127 - pss_ry) - (127 - pss_lx) + (127 - pss_ly)) / 4) * gear_dw; // 后右轮速度

        if (car_left1 > 30)
            car_left1 = 30; // 防溢出处理
        if (car_left1 < -30)
            car_left1 = -30;
        if (car_right1 > 30)
            car_right1 = 30;
        if (car_right1 < -30)
            car_right1 = -30;
        if (car_left2 > 30)
            car_left2 = 30;
        if (car_left2 < -30)
            car_left2 = -30;
        if (car_right2 > 30)
            car_right2 = 30;
        if (car_right2 < -30)
            car_right2 = -30;

        if ((car_left1_bak != car_left1) || (car_right1_bak != car_right1) || (car_left2_bak != car_left2) || (car_right2_bak != car_right2))
        {

            car_set(car_left1, car_right1, car_left2, car_right2);

            car_left1_bak = car_left1;
            car_right1_bak = car_right1;

            car_left2_bak = car_left2;
            car_right2_bak = car_right2;
        }
    }
}

// 处理手柄按键字符，buf为字符数组，mode是指模式 主要是红灯和绿灯模式
void parse_psx_buf(unsigned char *buf, unsigned char mode)
{
    u8 i, pos = 0;
    static u16 bak = 0xffff, temp, temp2;
    temp = (buf[0] << 8) + buf[1];

    if (bak != temp)
    {
        temp2 = temp;
        temp &= bak;
        for (i = 0; i < 16; i++)
        { // 16个按键一次轮询
            if ((1 << i) & temp)
            {
            }
            else
            {
                if ((1 << i) & bak)
                { // press 表示按键按下了

                    memset(uart_receive_buf, 0, sizeof(uart_receive_buf));
                    if (mode == PS2_LED_RED)
                    {
                        memcpy((char *)uart_receive_buf, (char *)pre_cmd_set_red[i], strlen(pre_cmd_set_red[i]));
                    }
                    else if (mode == PS2_LED_GRN)
                    {
                        memcpy((char *)uart_receive_buf, (char *)pre_cmd_set_grn[i], strlen(pre_cmd_set_grn[i]));
                    }
                    else
                        continue;
                    // zx_uart_send_str(uart_receive_buf);
                    pos = str_contain_str(uart_receive_buf, (u8 *)"^");
                    if (pos)
                        uart_receive_buf[pos - 1] = '\0';
                    if (str_contain_str(uart_receive_buf, (u8 *)"$"))
                    {
                        uart1_close();
                        uart1_get_ok = 0;
                        strcpy((char *)cmd_return, (char *)uart_receive_buf + 11);
                        strcpy((char *)uart_receive_buf, (char *)cmd_return);
                        uart1_get_ok = 1;
                        uart1_open();
                        uart1_mode = 1;
                    }
                    else if (str_contain_str(uart_receive_buf, (u8 *)"#"))
                    {
                        uart1_close();
                        uart1_get_ok = 0;
                        strcpy((char *)cmd_return, (char *)uart_receive_buf + 11);
                        strcpy((char *)uart_receive_buf, (char *)cmd_return);
                        uart1_get_ok = 1;
                        uart1_open();
                        uart1_mode = 2;
                    }
                    bak = 0xffff;
                }
                else
                { // release 表示按键松开了

                    memset(uart_receive_buf, 0, sizeof(uart_receive_buf));
                    if (mode == PS2_LED_RED)
                    {
                        memcpy((char *)uart_receive_buf, (char *)pre_cmd_set_red[i], strlen(pre_cmd_set_red[i]));
                    }
                    else if (mode == PS2_LED_GRN)
                    {
                        memcpy((char *)uart_receive_buf, (char *)pre_cmd_set_grn[i], strlen(pre_cmd_set_grn[i]));
                    }
                    else
                        continue;

                    pos = str_contain_str(uart_receive_buf, (u8 *)"^");
                    if (pos)
                    {
                        if (str_contain_str(uart_receive_buf + pos, (u8 *)"$"))
                        {
                            uart1_close();
                            uart1_get_ok = 0;
                            strcpy((char *)cmd_return, (char *)uart_receive_buf + pos);
                            cmd_return[strlen((char *)cmd_return) - 1] = '\0';
                            strcpy((char *)uart_receive_buf, (char *)cmd_return);
                            uart1_get_ok = 1;
                            uart1_mode = 1;
                        }
                        else if (str_contain_str(uart_receive_buf + pos, (u8 *)"#"))
                        {
                            uart1_close();
                            uart1_get_ok = 0;
                            strcpy((char *)cmd_return, (char *)uart_receive_buf + pos);
                            cmd_return[strlen((char *)cmd_return) - 1] = '\0';
                            strcpy((char *)uart_receive_buf, (char *)cmd_return);
                            uart1_get_ok = 1;
                            uart1_mode = 2;
                        }
                        // uart1_send_str(uart_receive_buf);
                    }
                }
            }
        }
        bak = temp2;
        beep_on();
        mdelay(10);
        beep_off();
    }
    return;
}
