#include "app_sensor.h"
#include "sensor/y_sensor.h"
#include "tcs34725/y_tcs34725.h"

static u8 flagSoundStart = 0;
static u8 tracking_refresh = 0;

void AI_xunji_moshi(void);
void AI_xunji_bizhang(void);
void AI_ziyou_bizhang(void);
void AI_yanse_shibie(void);
void AI_gensui_moshi(void);
void AI_dingju_jiaqu(void);
void AI_xunji_dingju(void);
void AI_shengkong_xunji(void);
void AI_shengkong_zhuaqu(void);

/**
 * @函数描述: 传感器相关设备控制初始化
 * @return {*}
 */
void app_sensor_init(void)
{
    ir_sensor_init();         /* 双路红外循迹初始化 */
    ultrasonic_sensor_init(); /* SR超声波初始化 */
    timer1_init(30000, 71);            /* 使能定时器，用于超声波计算 */

/* 不使用四路循迹模式，如果要使用四路循迹请把 IR4_TRACK_ENABLED 设为1 */
#if IR4_TRACK_ENABLED == 0
    AI_sensor_init();  // 声音识别静态识别夹取初始化
    ws2812b_init(WS2812B_ARR);    /* 幻彩灯初始化 */
    ws2812b_test(255); /* RGB幻彩测试函数 */
#endif

    // TCS34725_Init();/* 颜色传感器初始化 */
    if (TCS34725_Init())
    {
        printf("TCS34725_Init succeed\r\n");
    }
    else
    {
        printf("TCS34725_Init fail\r\n");
    }
}

/**
 * @函数描述: 循环检测输出传感器引脚的AD值
 * @return {*}
 */
void app_sensor_run(void)
{
    static u8 AI_mode_bak;

    // 有动作执行，直接返回
    if (group_do_ok == 0)
        return;

    if (AI_mode == 0)
    {
    }
    else if (AI_mode == 1)
    {
        AI_xunji_moshi(); /* 智能循迹模式 */
    }
    else if (AI_mode == 2)
    {
        AI_ziyou_bizhang(); // 自由避障
    }
    else if (AI_mode == 3)
    {
        AI_yanse_shibie(); /* 颜色识别 */
    }
    else if (AI_mode == 4)
    {
        AI_dingju_jiaqu(); // 定距夹取
    }
    else if (AI_mode == 5)
    {
        AI_gensui_moshi(); // 跟随功能
    }
    else if (AI_mode == 6)
    {
        AI_xunji_bizhang(); // 循迹避障
    }
    else if (AI_mode == 7)
    {
#if IR4_TRACK_ENABLED == 0
        AI_shengkong_zhuaqu(); /* 声控夹取 */
#endif
    }
    else if (AI_mode == 8)
    {
        AI_xunji_dingju(); // 循迹定距
    }
    else if (AI_mode == 9)
    {
        // 循迹识别
        AI_xunji_moshi();
        AI_yanse_shibie();
    }
    else if (AI_mode == 10)
    {
        AI_mode = 255;
    }

    if (AI_mode_bak != AI_mode)
    {
        AI_mode_bak = AI_mode;
        flagSoundStart = 0;
        group_do_ok = 1;
    }
}

/**
 * @函数描述: 声控抓取
 * @return {*}
 */
void AI_shengkong_zhuaqu(void)
{
    static uint8_t flag_num = 0;
    if (flag_num == 0) /* 第一声 */
    {
        // 静态识别夹取
        if (AI_Read() == 0) /* 如果要用触摸传感器请判断为1 */
        {
            mdelay(5);
            if (AI_Read() == 0)
            {
                flag_num = 1;
                parse_cmd((u8 *)"$DGT:18-26,1!");
                beep_on_times(1, 100);
            }
        }
    }
    else if (flag_num == 1) /* 第二声 */
    {
        // 静态识别夹取
        if (AI_Read() == 0) /* 如果要用触摸传感器请判断为1 */
        {
            mdelay(5);
            if (AI_Read() == 0)
            {
                flag_num = 0;
                parse_cmd((u8 *)"$DGT:9-17,1!");
                beep_on_times(1, 100);
            }
        }
    }
}

/*************************************************************
函数名称：AI_xunji_dingju()
功能介绍：在循迹的过程中实现定距夹取功能
函数参数：无
返回值：  无
*************************************************************/
void AI_xunji_dingju(void)
{
    if (group_do_ok == 0)
        return;

    AI_xunji_moshi();

    AI_dingju_jiaqu();
}

/*************************************************************
函数名称：AI_shengkong_xunji()
功能介绍：声控循迹函数
函数参数：无
返回值：  无
*************************************************************/
void AI_shengkong_xunji(void)
{

    if (AI_Read() == 0)
    {
        delay_ms(5);
        if (AI_Read() == 0)
        {
            flagSoundStart = 1;
            tracking_refresh = 1;
        }
    }

    if (flagSoundStart)
    {
        AI_xunji_moshi();
    }
}

/*************************************************************
函数名称：AI_dingju_jiaqu()
功能介绍：识别物体距离夹取物体
函数参数：无
返回值：  无
*************************************************************/
void AI_dingju_jiaqu(void)
{
    float adc_csb;
    if (group_do_ok == 0)
        return;
	
    adc_csb = sensor_sr_ultrasonic_read(); // 获取a0的ad值，计算出距离
    if ((adc_csb > 14.0) && (adc_csb <= 16.0))
    {
        tracking_refresh = 1;
        car_set(0, 0, 0, 0);
        adc_csb = sensor_sr_ultrasonic_read(); // 获取a0的ad值，计算出距离
        if ((adc_csb > 14.0) && (adc_csb <= 15.5))
        {
            adc_csb = sensor_sr_ultrasonic_read(); // 获取a0的ad值，计算出距离
            if ((adc_csb > 14.0) && (adc_csb <= 15.5))
            {
                // 距离15cm左右就夹取
                car_set(0, 0, 0, 0);
                beep_on_times(1, 100);
                parse_cmd((u8 *)"$DGT:27-35,1!");
            }
        }
    }
}

/*************************************************************
函数名称：AI_gensui_moshi()
功能介绍：检测物体距离，在一定距离内实现跟随功能
函数参数：无
返回值：  无
*************************************************************/
void AI_gensui_moshi(void)
{
    static u32 systick_ms_bak = 0;
    int speed = 12, adc_csb;
    if (millis() - systick_ms_bak > 100)
    {
        systick_ms_bak = millis();
        adc_csb = sensor_sr_ultrasonic_read(); // 获取a0的ad值，计算出距离

        if ((adc_csb > 30) && (adc_csb < 50))
        {
            // 距离30~50cm前进
            car_set(speed, speed, speed, speed);
        }
        else if ((adc_csb < 20) && (adc_csb > 0))
        {
            // 距离低于20cm就后退
            car_set(-speed, -speed, -speed, -speed);
        }
        else
        {
            // 其他情况停止
            car_set(0, 0, 0, 0);
        }
    }
}

/*************************************************************
函数名称：AI_yanse_shibie()
功能介绍：识别木块颜色，夹取分别放到不同位置
函数参数：无
返回值：  无
******************************************************** *****/
void AI_yanse_shibie(void)
{
    static u32 systick_ms_yanse = 0;

    if (group_do_ok && millis() - systick_ms_yanse > 20)
    {
        systick_ms_yanse = millis();
        TCS34725_LedON(0); /* 关闭颜色识别传感器LED */
        TCS34725_GetRawData(&rgb); // 获取RGB
        if (rgb.c < 1)
        {
            tracking_refresh = 1;
            car_set(0, 0, 0, 0);
            beep_on_times(1, 100);
            TCS34725_LedON(1); // 打开LED
            delay_ms(800);
            TCS34725_GetRawData(&rgb);          // 获取RGB
            if (rgb.r > rgb.g && rgb.r > rgb.b) /* 红 前 */
            {
                car_set(0, 0, 0, 0);
                parse_cmd("$DGT:2-8,1!"); // 执行脱机存储动作组
            }
            else if (rgb.g > rgb.r && rgb.g > rgb.b) /* 绿 右 */
            {
                car_set(0, 0, 0, 0);
                parse_cmd("$DGT:9-17,1!"); // 执行脱机存储动作组
            }
            else if (rgb.b > rgb.g && rgb.b > rgb.r) /* 蓝 左 */
            {
                car_set(0, 0, 0, 0);
                parse_cmd("$DGT:18-26,1!"); // 执行脱机存储动作组
            }
        }
    }
}

/*************************************************************
函数名称：AI_ziyou_bizhang()
功能介绍：识别物体距离从而避开物体前进
函数参数：无
返回值：  无
*************************************************************/
void AI_ziyou_bizhang(void)
{
    static u32 systick_ms_bak = 0;
    static int bz_num = 0, bz_num_bak = 0;
    int speed = 14, adc_csb;
    if (millis() - systick_ms_bak > 100)
    {
        systick_ms_bak = millis();
        adc_csb = (int)sensor_sr_ultrasonic_read(); // 获取a0的ad值，计算出距离

        if ((adc_csb < 35) && (adc_csb > 0))
        {
            delay_ms(50);
            adc_csb = (int)sensor_sr_ultrasonic_read(); // 获取a0的ad值，计算出距离
            if ((adc_csb < 35) && (adc_csb > 0))
            {
                // 距离低于35cm就右转
                if (adc_csb < 25)
                {
                    bz_num = 1;
                    // delay_ms(200);
                }
                else
                {
                    bz_num = 2;
                }
            }
        }
        else
        {
            bz_num = 3;
        }
        if (bz_num != bz_num_bak)
        {
            switch (bz_num)
            {
            case 1:
                car_set(-speed, -speed, -speed, -speed);
                break;
            case 2:
                car_set(speed, -speed, speed, -speed);
                delay_ms(800);
                break;
            case 3:
                car_set(speed, speed, speed, speed);
                break;
            default:
                zx_uart_send_str("error\n");
                break;
            }
            bz_num_bak = bz_num;
        }
    }
}

/*************************************************************
函数名称：AI_xunji_moshi()
功能介绍：实现循迹功能
函数参数：无
返回值：  无
*************************************************************/
void AI_xunji_moshi(void)
{
    bool xj0, xj1;
    static int speed_xj = 13;
    static int xj_num = 0, xj_num_bak = 0;

/* 是否四路循迹 */
#if IR4_TRACK_ENABLED

    static uint8_t intersection_judgment = 0;
    static uint8_t judgment_flag = 0;

    if (AI_mode == 1) /* 如果等于智能循迹 */
    {
        if (IR4_LEFT_READ() && IR4_RIGHT_READ())
        {
            if (judgment_flag == 1) /* 用来判断是否通过路口 */
            {
                delay_ms(10);
                if (IR4_LEFT_READ() && IR4_RIGHT_READ())
                {
                    judgment_flag = 0;
                    car_set(0, 0, 0, 0);
                    if (intersection_judgment == 0) /* 第一个路口 */
                    {
                        intersection_judgment = 1;
                        xj_num_bak = 0;             /* 刷新记录 */
                        parse_cmd("$DGT:18-26,1!"); // 左放
                    }
                    else if (intersection_judgment == 1) /* 第二个路口 */
                    {
                        intersection_judgment = 2;
                        xj_num_bak = 0;             /* 刷新记录 */
                        parse_cmd("$DGT:18-26,1!"); // 左放
                    }
                    else if (intersection_judgment == 2) /* 第三个路口 */
                    {
                        intersection_judgment = 0;
                        xj_num_bak = 0; /* 刷新记录 */
                        AI_mode = 255;  /* 停止循迹 */
                    }

                    return;
                }
            }
        }
        else if ((IR4_LEFT_READ() == 0) && (IR4_RIGHT_READ() == 0)) /* 路口已通过 */
        {
            judgment_flag = 1;
        }
    }

    if ((IR4_LEFT_READ()) && (!IR4_RIGHT_READ())) /* 左边信号 */
    {
        car_set(-0, speed_xj, -0, speed_xj);
    }
    else if ((!IR4_LEFT_READ()) && (IR4_RIGHT_READ())) /* 右边信号 */
    {
        car_set(speed_xj, -0, speed_xj, -0);
    }
    else if (IR4_LEFT_READ() && IR4_RIGHT_READ())
    {
        car_set(speed_xj, speed_xj, speed_xj, speed_xj);
    }

#endif

    xj0 = !IR_LEFT_READ(); /* 读取循迹传感器引脚电平 */
    xj1 = !IR_RIGHT_READ();
    if (tracking_refresh == 1) /* 如果不等于等于智能循迹 */
    {
        tracking_refresh = 0;
        xj_num = 0;
        xj_num_bak = 0;
    }

    if (xj0 && xj1)
    {
        xj_num = 1;
    }
    else if (xj0)
    {
        xj_num = 2;
    }
    else if (xj1)
    {
        xj_num = 3;
    }
    else
    {
        xj_num = 4;
    }

    if (xj_num != xj_num_bak)
    {
        switch (xj_num)
        {
        case 1: /* 正常 */
            car_set(speed_xj, speed_xj, speed_xj, speed_xj);
            break;
        case 2: /* 右边信号 */
            car_set(-0, speed_xj, -0, speed_xj);
            break;
        case 3: /* 左边信号 */
            car_set(speed_xj, -0, speed_xj, -0);
            break;
        // case 4: car_set(0,0,0,0); break;
        default: /* 超出测量距离 */
            zx_uart_send_str("error\n");
            break;
        }
        xj_num_bak = xj_num;
    }
}

/*************************************************************
函数名称：AI_xunji_bizhang()
功能介绍：在循迹的过程中，检测有障碍物，则停止，否则继续循迹
函数参数：无
返回值：  无
*************************************************************/
void AI_xunji_bizhang(void)
{
    static u32 systick_ms_bak = 0;
    int adc_csb;
    static bool flag = 0;

    if (millis() - systick_ms_bak > 50)
    {
        systick_ms_bak = millis();
        // 避障处理
        adc_csb = (int)sensor_sr_ultrasonic_read(); // 获取a0的ad值，计算出距离
        if ((adc_csb < 15) && (adc_csb > 0))
        { // 距离低于15cm就停止
            car_set(0, 0, 0, 0);
            flag = 1;
        }
        else
        {
            // 循迹处理
            if (flag == 1)
            {
                car_set(12,12,12,12);
                flag = 0;
            }
            AI_xunji_moshi();
        }
    }
}
