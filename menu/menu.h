#ifndef CODE_MENU_MENU_H_
#define CODE_MENU_MENU_H_

/* 旋钮按键与菜单显示参数。 */
#define MENU_KEY             MY_KEY_1
#define MENU_ROW_HEIGHT      (18U)
#define MENU_TEXT_COLOR      RGB565_WHITE
#define MENU_SELECTED_COLOR  RGB565_RED

/* 编码器和电机测试页共用的数据刷新周期，单位毫秒。 */
#define MENU_ENCODER_REFRESH_MS (10U)

void Menu_Encoder_Init(void);
void Menu_Encoder_Run(void);

#endif /* CODE_MENU_MENU_H_ */
