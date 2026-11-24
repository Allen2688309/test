#include "MyHeadFile.h"

static uint32 Display_interval_ticks;
static uint32 Last_display_tick;

static void Menu_Show_Encoder_Data(void);

typedef enum
{
    MENU_PAGE_HOME,
    MENU_PAGE_ENCODER,
    MENU_PAGE_MOTOR,
    MENU_PAGE_MOTION
} Menu_page_id_t;

/* 第 0 项为停机，之后按 m-motor.h 中的轮位顺序排列。 */
static const char *const Motor_items[] = {
    "STOP MOTORS", "UL J4", "UR J3", "LL J1", "LR J2"
};


typedef struct
{
    const char *Label;
    uint8 (*Set_direction)(void);
} Menu_motion_item_t;

static const Menu_motion_item_t Motion_items[] = {
    {"FORWARD", Dir_Forward},
    {"BACKWARD", Dir_Backward},
    {"MOVE LEFT", Dir_Left},
    {"MOVE RIGHT", Dir_Right},
    {"TURN LEFT", Dir_Turn_Left},
    {"TURN RIGHT", Dir_Turn_Right}
};

static const char *const Page_titles[] = {
    "MECANUM TEST",
    "ENCODER TEST",
    "MOTOR TEST",
    "MOTION TEST"
};

static const Menu_page_id_t Home_pages[] = {
    MENU_PAGE_ENCODER,
    MENU_PAGE_MOTOR,
    MENU_PAGE_MOTION
};

static Menu_page_id_t Current_page;
static uint8 Home_selection;
static uint8 Motor_selection;
static uint8 Motion_selection;
static uint8 Running_motion;

// 函数简介       重画当前页的菜单选项，并高亮选中行。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
static void Menu_Draw_Rows(void)
{
    uint8 Row;
    uint16 Color;

    if (Current_page == MENU_PAGE_HOME)
    {
        for (Row = 0U; Row < sizeof(Home_pages) / sizeof(Home_pages[0]); ++Row)
        {
            Color = MENU_TEXT_COLOR;
            if (Row == Home_selection)
            {
                Color = MENU_SELECTED_COLOR;
            }
            ips200_show_string_color(0, (Row + 1U) * MENU_ROW_HEIGHT,
                                     Page_titles[Home_pages[Row]], Color);
        }
    }
    else if (Current_page == MENU_PAGE_MOTOR)
    {
        for (Row = 0U; Row < sizeof(Motor_items) / sizeof(Motor_items[0]); ++Row)
        {
            Color = MENU_TEXT_COLOR;
            if (Row == Motor_selection)
            {
                Color = MENU_SELECTED_COLOR;
            }
            ips200_show_string_color(0, (Row + 1U) * MENU_ROW_HEIGHT,
                                     Motor_items[Row], Color);
        }
    }
    else if (Current_page == MENU_PAGE_MOTION)
    {
        for (Row = 0U; Row < sizeof(Motion_items) / sizeof(Motion_items[0]); ++Row)
        {
            Color = MENU_TEXT_COLOR;
            if (Row == Motion_selection)
            {
                Color = MENU_SELECTED_COLOR;
            }
            ips200_show_string_color(0, (Row + 1U) * MENU_ROW_HEIGHT,
                                     Motion_items[Row].Label, Color);
        }
    }
}

// 函数简介       切换菜单页面并重画屏幕，离开单轮或整车测试页时先停止电机。
// 参数说明       Page：要打开的菜单页面。
// return         无。
//date  : 2026-11-24
//author: Q
static void Menu_Open_Page(Menu_page_id_t Page)
{
    if (Current_page == MENU_PAGE_MOTOR || Current_page == MENU_PAGE_MOTION)
    {
        M_Motor_Stop_All();
    }

    Current_page = Page;
    Last_display_tick = IfxStm_getLower(&MODULE_STM0) - Display_interval_ticks;
    ips200_clear();
    ips200_show_string_centered(0, Page_titles[Current_page]);
    Menu_Draw_Rows();

    if (Current_page == MENU_PAGE_MOTOR)
    {
        ips200_show_string(0, 300, "SELECT WHEEL AGAIN TO REVERSE");
    }
    else if (Current_page == MENU_PAGE_MOTION)
    {
        ips200_show_string(0, 300, "PRESS AGAIN TO STOP");
    }
}

// 函数简介       初始化 IPS、菜单光标和显示刷新周期，确认停机后打开首页。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void Menu_Encoder_Init(void)
{
    Current_page = MENU_PAGE_HOME;
    Home_selection = 0U;
    Motor_selection = 0U;
    Motion_selection = 0U;
    Running_motion = 0U;

    ips200_init(IPS200_TYPE_SPI);
    ips200_set_dir(IPS200_PORTAIT);
    ips200_set_font(IPS200_8X16_FONT);
    Display_interval_ticks = (uint32)((uint64)IfxStm_getFrequency(&MODULE_STM0)
                                     * MENU_ENCODER_REFRESH_MS / 1000U);
    M_Motor_Stop_All();
    Menu_Open_Page(MENU_PAGE_HOME);
}

// 函数简介       按旋钮转动的格数循环移动菜单光标。
// 参数说明       Selection：原选中行的索引，从零开始。
// 参数说明       Steps：旋钮转动格数，可为负数。
// 参数说明       Count：可选行数，必须大于零。
// return         移动后的行索引，范围为零到 Count 减一。
//date  : 2026-11-24
//author: Q
static uint8 Menu_Move_Selection(uint8 Selection, int16 Steps, uint8 Count)
{
    int32 Next_selection = ((int32)Selection + Steps) % (int32)Count;

    if (Next_selection < 0)
    {
        Next_selection += Count;
    }
    return (uint8)Next_selection;
}

// 函数简介       处理旋钮和按键事件，执行菜单操作并刷新单轮或整车测试数据。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void Menu_Encoder_Run(void)
{
    uint8 Key_event = my_key_get_state(MENU_KEY);
    uint8 Knob_changed = If_Switch_Encoder_Change();

    if (Key_event == MY_KEY_LONG_PRESS && Current_page != MENU_PAGE_HOME)
    {
        Menu_Open_Page(MENU_PAGE_HOME);
        return;
    }

    if (Knob_changed != 0U)
    {
        if (Current_page == MENU_PAGE_HOME)
        {
            Home_selection = Menu_Move_Selection(Home_selection, switch_encoder_change_num,
                                                sizeof(Home_pages) / sizeof(Home_pages[0]));
            Menu_Draw_Rows();
        }
        else if (Current_page == MENU_PAGE_MOTOR)
        {
            Motor_selection = Menu_Move_Selection(Motor_selection, switch_encoder_change_num,
                                                 sizeof(Motor_items) / sizeof(Motor_items[0]));
            Menu_Draw_Rows();
        }
        else if (Current_page == MENU_PAGE_MOTION)
        {
            Motion_selection = Menu_Move_Selection(Motion_selection, switch_encoder_change_num,
                                                  sizeof(Motion_items) / sizeof(Motion_items[0]));
            Menu_Draw_Rows();
        }
    }

    if (Key_event == MY_KEY_SHORT_PRESS)
    {
        if (Current_page == MENU_PAGE_HOME)
        {
            Menu_Open_Page(Home_pages[Home_selection]);
        }
        else if (Current_page == MENU_PAGE_MOTOR)
        {
            if (Motor_selection == 0U)
            {
                M_Motor_Stop_All();
            }
            else
            {
                M_Motor_Test_Select(Motor_selection - 1U);
            }
        }
        else if (Current_page == MENU_PAGE_MOTION)
        {
            if (Motion_selection == Running_motion && M_Motor_Get_Active_Mask() != 0U)
            {
                M_Motor_Stop_All();
            }
            else
            {
                /* 方向函数先清零原来的输出，再写四轮方向。 */
                uint8 Wheel_mask = Motion_items[Motion_selection].Set_direction();
                M_Motor_Start_Motion(Wheel_mask);
                Running_motion = Motion_selection;
            }
        }
    }

    if (Current_page != MENU_PAGE_HOME)
    {
        Menu_Show_Encoder_Data();
    }
}

// 函数简介       按屏幕刷新周期显示四轮计数、转速、平均净位移和电机测试状态。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
static void Menu_Show_Encoder_Data(void)
{
    static const uint8 Wheel_index[ENCODER_CHANNEL_COUNT] = {3, 2, 0, 1};
    static const char *Wheel_name[ENCODER_CHANNEL_COUNT] = {
        "UL J4", "UR J3", "LL J1", "LR J2"
    };
    uint8 Index;
    uint8 Active_Wheel;
    uint16 Y;
    uint32 Now_tick = IfxStm_getLower(&MODULE_STM0);
    int32 Count[ENCODER_CHANNEL_COUNT];
    float Speed_rps[ENCODER_CHANNEL_COUNT];
    uint16 Count_x;

    if (Now_tick - Last_display_tick < Display_interval_ticks)
    {
        return;
    }
    Last_display_tick = Now_tick;

    Encoder_GetSnapshot(Count, Speed_rps);
    Active_Wheel = M_Motor_Get_Active_Wheel();
    ips200_set_color(RGB565_WHITE, RGB565_BLACK);
    if (Current_page == MENU_PAGE_MOTION)
    {
        ips200_show_string(0, 126, "     COUNT      r/s    TGT PWM");
        Count_x = 40U;
    }
    else
    {
        ips200_show_string(0, 126, "COUNT / SPEED r/s");
        Count_x = 48U;
    }

    for (Index = 0; Index < ENCODER_CHANNEL_COUNT; ++Index)
    {
        Y = 148U + 22U * Index;
        ips200_show_string(0, Y, Wheel_name[Index]);
        if (Current_page != MENU_PAGE_MOTION)
        {
            ips200_show_string(40, Y, "C");
        }
        if (Count[Wheel_index[Index]] == INT32_MIN)
        {
            ips200_show_string(Count_x, Y, "-2147483648");
        }
        else
        {
            ips200_show_int(Count_x, Y, Count[Wheel_index[Index]], 10);
        }

        if (Current_page == MENU_PAGE_MOTION)
        {
            ips200_show_float(128, Y, Speed_rps[Wheel_index[Index]], 3, 2);
            ips200_show_uint(184, Y, M_Motor_Get_Target(Index), 3);
            ips200_show_uint(208, Y, M_Motor_Get_PWM(Index), 4);
        }
        else
        {
            ips200_show_string(144, Y, "V");
            ips200_show_float(152, Y, Speed_rps[Wheel_index[Index]], 3, 2);
        }
    }

    if (Current_page == MENU_PAGE_MOTION)
    {
        uint8 Active_mask = M_Motor_Get_Active_Mask();
        const char *Motion_label = "STOPPED";
        char Status[31];

        if (Active_mask != 0U)
        {
            Motion_label = Motion_items[Running_motion].Label;
        }
        snprintf(Status, sizeof(Status), "%-12s %2u r/s", Motion_label,
                 (unsigned int)M_Motor_Get_Target_RPS());
        ips200_show_string(0, 246, Status);
        ips200_show_string(0, 264, "DIST AVG mm");
        ips200_show_float(104, 264, Encoder_GetDistanceMM(), 8, 2);
        ips200_show_string(0, 282, "PRESS AGAIN TO STOP");
        return;
    }

    ips200_show_string(0, 246, "DIST AVG mm");
    ips200_show_float(104, 246, Encoder_GetDistanceMM(), 8, 2);
    ips200_show_string(0, 282, "TEST:");
    ips200_show_string(0, 264, "TGT r/s:");
    if (Active_Wheel < M_MOTOR_WHEEL_COUNT)
    {
        ips200_show_int(64, 264, M_Motor_Get_Single_Target(), 5);
        ips200_show_string(48, 282, Wheel_name[Active_Wheel]);
        ips200_show_string(104, 282, "DIR:");
        ips200_show_uint(136, 282, M_Motor_Get_Direction(Active_Wheel), 1);
        ips200_show_string(160, 282, "PWM:");
        ips200_show_uint(200, 282, M_Motor_Get_PWM(Active_Wheel), 4);
    }
    else
    {
        ips200_show_int(64, 264, 0, 5);
        ips200_show_string(48, 282, "STOPPED                 ");
    }
}
