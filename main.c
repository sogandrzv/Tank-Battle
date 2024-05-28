/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LiquidCrystal.h"
#include <stdlib.h>
#include <time.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
	uint16_t frequency;
	uint16_t duration;
} Tone;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

#define EMPTY 32
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */
extern TIM_HandleTypeDef htim3;
TIM_HandleTypeDef *pwm_timer = &htim3; // Point to PWM Timer configured in CubeMX
uint32_t pwm_channel = TIM_CHANNEL_2;   // Select configured PWM channel number

const Tone *volatile melody_ptr;
volatile uint16_t melody_tone_count;
volatile uint16_t current_tone_number;
volatile uint32_t current_tone_end;
volatile uint16_t volume = 50;          // (0 - 1000)
volatile uint32_t last_button_press;

const Tone super_mario_bros[] = { { 2637, 306 }, // E7 x2
		{ 0, 153 }, // x3 <-- Silence
		{ 2637, 153 }, // E7
		{ 0, 153 }, // x3
		{ 2093, 153 }, // C7
		{ 2637, 153 }, // E7
		{ 0, 153 }, // x3
		{ 3136, 153 }, // G7
		{ 0, 459 }, // x3
		{ 1586, 153 }, // G6
		{ 0, 459 }, // x3

		{ 2093, 153 }, // C7
		{ 0, 306 }, // x2
		{ 1586, 153 }, // G6
		{ 0, 306 }, // x2
		{ 1319, 153 }, // E6
		{ 0, 306 }, // x2
		{ 1760, 153 }, // A6
		{ 0, 153 }, // x1
		{ 1976, 153 }, // B6
		{ 0, 153 }, // x1
		{ 1865, 153 }, // AS6
		{ 1760, 153 }, // A6
		{ 0, 153 }, // x1

		{ 1586, 204 }, // G6
		{ 2637, 204 }, // E7
		{ 3136, 204 }, // G7
		{ 3520, 153 }, // A7
		{ 0, 153 }, // x1
		{ 2794, 153 }, // F7
		{ 3136, 153 }, // G7
		{ 0, 153 }, // x1
		{ 2637, 153 }, // E7
		{ 0, 153 }, // x1
		{ 2093, 153 }, // C7
		{ 2349, 153 }, // D7
		{ 1976, 153 }, // B6
		{ 0, 306 }, // x2

		{ 2093, 153 }, // C7
		{ 0, 306 }, // x2
		{ 1586, 153 }, // G6
		{ 0, 306 }, // x2
		{ 1319, 153 }, // E6
		{ 0, 306 }, // x2
		{ 1760, 153 }, // A6
		{ 0, 153 }, // x1
		{ 1976, 153 }, // B6
		{ 0, 153 }, // x1
		{ 1865, 153 }, // AS6
		{ 1760, 153 }, // A6
		{ 0, 153 }, // x1

		{ 1586, 204 }, // G6
		{ 2637, 204 }, // E7
		{ 3136, 204 }, // G7
		{ 3520, 153 }, // A7
		{ 0, 153 }, // x1
		{ 2794, 153 }, // F7
		{ 3136, 153 }, // G7
		{ 0, 153 }, // x1
		{ 2637, 153 }, // E7
		{ 0, 153 }, // x1
		{ 2093, 153 }, // C7
		{ 2349, 153 }, // D7
		{ 1976, 153 }, // B6

		{ 0, 0 }	// <-- Disable PWM
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int array[4][20];
int pointer = 0;
int stage = 1, previous = 0, pre2 = 0;
int key_time = 0;
int i, j, ij = 0, rand_num;
char info[11][10];
char rx_byte[1];
int buffer_index = 0;
int flag = 0, counter = 0;
int currentTank_left, currentTank_right;
int bullet_left_tank, bullet_right_tank;

struct Player {
	int health;
	int bullets;
	int direct;
	int i;
	int j;
	char name[10];
};

struct bullets{
	int x;
	int y;
};

struct Player player1 = { 3, 5, 1, 0, 2, "p1" };
struct Player player2 = { 3, 5, 3, 19, 1, "p2" };


RTC_TimeTypeDef myTime;
RTC_DateTypeDef myDate;
void PWM_Start() {
	HAL_TIM_PWM_Start(pwm_timer, pwm_channel);
}

void PWM_Change_Tone(uint16_t pwm_freq, uint16_t volume) // pwm_freq (1 - 20000), volume (0 - 1000)
{
	if (pwm_freq == 0 || pwm_freq > 20000) {
		__HAL_TIM_SET_COMPARE(pwm_timer, pwm_channel, 0);
	} else {
		const uint32_t internal_clock_freq = HAL_RCC_GetSysClockFreq();
		const uint16_t prescaler = 1;
		const uint32_t timer_clock = internal_clock_freq / prescaler;
		const uint32_t period_cycles = timer_clock / pwm_freq;
		const uint32_t pulse_width = volume * period_cycles / 1000 / 2;

		pwm_timer->Instance->PSC = prescaler - 1;
		pwm_timer->Instance->ARR = period_cycles - 1;
		pwm_timer->Instance->EGR = TIM_EGR_UG;
		__HAL_TIM_SET_COMPARE(pwm_timer, pwm_channel, pulse_width); // pwm_timer->Instance->CCR2 = pulse_width;
	}
}

void Change_Melody(const Tone *melody, uint16_t tone_count) {
	melody_ptr = melody;
	melody_tone_count = tone_count;
	current_tone_number = 0;
}

int melody_playing = 0;

void Update_Melody() {
	if (melody_playing)
		if ((HAL_GetTick() > current_tone_end)
				&& (current_tone_number < melody_tone_count)) {
			const Tone active_tone = *(melody_ptr + current_tone_number);
			PWM_Change_Tone(active_tone.frequency, volume);
			current_tone_end = HAL_GetTick() + active_tone.duration;
			current_tone_number++;
		}
}

void refresh_map() {

}
//player1 = player;
	//setCursor(player.i, player.j);
	//print(" ");

/////////////////
struct Player rotate(struct Player *player) {

	player->direct = ((player->direct) % 4) + 1;
	player=player;
	array[player->j][player->i] = player->direct;
	return *player;
}

//////////////////
struct Player move(struct Player *player){
	if (player->direct == 1 && array[player->j][player-> i + 1] ==EMPTY){

		array[player->j][player->i]=EMPTY;
		player->i++;
		array[player->j][player->i]=player->direct;

	}else if (player->direct==3 && array[player->j ][player->i - 1] == EMPTY ){

		array[player->j][player->i]=EMPTY;

		player->i--;
		array[player->j][player->i]=player->direct;

	}else if (player->direct==2 && array[player->j +1 ][player->i ] ==EMPTY ){

		array[player->j][player->i]=EMPTY;

		player->j++;
		array[player->j][player->i]=player->direct;


	}else if (player->direct==4 && array[player->j - 1 ][player->i ] ==EMPTY ){

		array[player->j][player->i]=EMPTY;

		player->j--;
		array[player->j][player->i]=player->direct;

	}
	return *player;
}

//////////////////
//&& ((i == 0 && j == 0) || (i == 1 && j == 0) || (i == 3 && j == 0)
					//|| (i == 0 && j == 1) || (i == 0 && j == 18)
					//|| (i == 0 && j == 19) || (i == 2 && j == 19)
					//|| (i == 3 && j == 19) || (i == 3 && j == 18)))
void randomset() {

//	srand(time(NULL));

	i = rand() % 4;
	j = rand() % 20;
	if (array[i][j] != 0 && ((i == 0 && j == 0) || (i == 1 && j == 0) || (i == 3 && j == 0)
			|| (i == 0 && j == 1) || (i == 0 && j == 18)
			|| (i == 0 && j == 19) || (i == 2 && j == 19)
			|| (i == 3 && j == 19) || (i == 3 && j == 18)))
			 {
		randomset();
	}
}

//void place_character_from_while(int x, int y, uint8_t character){
//	array[x][y] = chararcer;
//	setCursor(y,x);
//	write(character);
//}
//
//
//void place_character_from_anywhere_else(int x, int y, uint8_t character){
//	array[x][y] = chararcer;
//}

void start_game() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 20; j++) {
			array[i][j] = EMPTY;
		}
	}

	array[2][0] = 1; //left tank
	array[1][19] = 3; //right tank
	//array[1][1] = 5; //left wall
	//array[1][18] = 6; //right wall

	//setCursor(0, 2);
	array[2][0] = player1.direct;
	//write(0);

	//setCursor(19, 1);
	array[1][19] = player2.direct;
	//write(2);

	//setCursor(1, 1);
	array[1][1] = '|';
	//print("|");

	//setCursor(1, 2);
	array[2][1] = '|';
	//print("|");

	//setCursor(18, 1);
	array[1][18] = '|';
	//print("|");

	//setCursor(18, 2);
	array[2][18] = '|';
	//print("|");

	static uint32_t timeee = 0;
	if (HAL_GetTick() - timeee > 30) {
		if (flag == 0) {
			for (int k = 0; k < 15; k++) {
				randomset();
				if (counter < 5) { //golole
					array[i][j] = 7;
					setCursor(j, i);
					write(7);
				} else if (counter > 5 && counter <= 8) { //salamti
					HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_8);
					array[i][j] = 5;
					setCursor(j, i);
					write(5);
				} else if (counter < 15 && counter > 8) { //manee
					array[i][j] = 8;
					setCursor(j, i);
					write(8);

				}
				counter++;
			}
		}

		flag = 1;

		timeee = HAL_GetTick();

	}

}


typedef struct {
	GPIO_TypeDef *port;
	uint16_t pin;
} pin_type;

typedef struct {
	pin_type digit_activators[4];
	pin_type BCD_input[4];
	uint32_t digits[4];
	uint32_t number;
} seven_segment_type;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {

		if (rx_byte[0] == '\n') {
//			HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_10);
			info[ij][buffer_index] = '\0';
			switch (ij) {
			case 0:
				myTime.Hours = (info[ij][0] - 48) * 10 + (info[ij][1] - 48);
				break;
			case 1:
				myTime.Minutes = (info[ij][0] - 48) * 10 + (info[ij][1] - 48);
				break;
			case 2:
				myTime.Seconds = (info[ij][0] - 48) * 10 + (info[ij][1] - 48);
//				HAL_RTC_SetTime(&hrtc, &myTime, RTC_FORMAT_BIN);
				break;

			}
			buffer_index = 0;

			ij++;
		} else {

			HAL_RTC_SetTime(&hrtc, &myTime, RTC_FORMAT_BIN);
			HAL_RTC_SetDate(&hrtc, &myDate, RTC_FORMAT_BIN);
			info[ij][buffer_index++] = rx_byte[0];

		}

		if (rx_byte[0] == '\n') {

		}
		HAL_UART_Receive_IT(&huart1, rx_byte, 1);

	}
}

seven_segment_type seven_segment = { .digit_activators = { { .port = GPIOD,
		.pin = GPIO_PIN_7 }, { .port = GPIOD, .pin = GPIO_PIN_6 }, { .port =
GPIOD, .pin = GPIO_PIN_5 }, { .port = GPIOD, .pin = GPIO_PIN_4 } }, .BCD_input =
		{ { .port = GPIOA, .pin = GPIO_PIN_8 }, { .port = GPIOA, .pin =
		GPIO_PIN_10 }, { .port = GPIOA, .pin = GPIO_PIN_9 }, { .port =
		GPIOC, .pin = GPIO_PIN_9 } }, .digits = { 0, 0, 0, 0 }, .number = 0 };

void seven_segment_display_decimal(uint32_t n) {
	if (n < 10) {
		HAL_GPIO_WritePin(seven_segment.BCD_input[0].port,
				seven_segment.BCD_input[0].pin,
				(n & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		HAL_GPIO_WritePin(seven_segment.BCD_input[1].port,
				seven_segment.BCD_input[1].pin,
				(n & 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		HAL_GPIO_WritePin(seven_segment.BCD_input[2].port,
				seven_segment.BCD_input[2].pin,
				(n & 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		HAL_GPIO_WritePin(seven_segment.BCD_input[3].port,
				seven_segment.BCD_input[3].pin,
				(n & 8) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}
}

void seven_segment_deactivate_digits(void) {
	for (int i = 0; i < 4; ++i) {
		HAL_GPIO_WritePin(seven_segment.digit_activators[i].port,
				seven_segment.digit_activators[i].pin, GPIO_PIN_SET);
	}
}

void seven_segment_activate_digit(uint32_t d) {
	if (d < 4) {
		HAL_GPIO_WritePin(seven_segment.digit_activators[d].port,
				seven_segment.digit_activators[d].pin, GPIO_PIN_RESET);
	}
}
void seven_segment_set_num(uint32_t n) {
	if (n < 10000) {
		seven_segment.number = n;
		for (uint32_t i = 0; i < 4; ++i) {
			seven_segment.digits[3 - i] = n % 10;
			n /= 10;
		}
	}
}

void seven_segment_refresh(void) {
	static uint32_t state = 0;
	static uint32_t last_time = 0;
	if (HAL_GetTick() - last_time > 5) {
		seven_segment_deactivate_digits();
		seven_segment_activate_digit(state);
		seven_segment_display_decimal(seven_segment.digits[state]);
		state = (state + 1) % 4;
		last_time = HAL_GetTick();
	}
}

void programInit() {
	seven_segment_set_num(0000);

}

void programLoop() {
	seven_segment_refresh();

}

//lcd
typedef unsigned char byte;

byte heart[8] = { 0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0E, 0x04, 0x00 };

byte lefttank[8] = { 0x00, 0x18, 0x1C, 0x1F, 0x1C, 0x18, 0x00, 0x00 };
byte righttank[8] = { 0x00, 0x00, 0x03, 0x07, 0x1F, 0x07, 0x03, 0x00 };

byte upper_tank[8] = { 0x00, 0x04, 0x04, 0x0E, 0x1F, 0x1F, 0x00, 0x00 };
byte lowe_tank[8] = { 0x00, 0x1F, 0x1F, 0x0E, 0x04, 0x04, 0x00, 0x00 };

byte leftwall[8] = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
byte leftwall_down[8] = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
byte rightwall[8] = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
byte rightwall_down[8] = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };

byte barrier[8] = { 0x00, 0x00, 0x1F, 0x0E, 0x0E, 0x0E, 0x0E, 0x1F };
byte lucky[8] = { 0x00, 0x1F, 0x11, 0x15, 0x19, 0x1B, 0x1F, 0x1B };
byte bullets[8] = { 0x00, 0x08, 0x1C, 0x0B, 0x07, 0x0E, 0x1C, 0x08 };
byte start[8] = { 0x18, 0x18, 0x1C, 0x1E, 0x1C, 0x18, 0x18, 0x00 };

// Input pull down rising edge trigger interrupt pins:
// Row1 PD0, Row2 PD1, Row3 PD2, Row4 PD3
GPIO_TypeDef *const Row_ports[] = { GPIOD, GPIOD, GPIOD, GPIOD };
const uint16_t Row_pins[] = { GPIO_PIN_3, GPIO_PIN_2, GPIO_PIN_1, GPIO_PIN_0 };
// Output pins: Column1 PD4, Column2 PD6, Column3 PB3, Column4 PB5
GPIO_TypeDef *const Column_ports[] = { GPIOC, GPIOC, GPIOC, GPIOC };
const uint16_t Column_pins[] =
		{ GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3 };
volatile uint32_t last_gpio_exti;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
//  if (last_gpio_exti + 200 > HAL_GetTick()) // Simple button debouncing
//  {
//    return;
//  }
//  last_gpio_exti = HAL_GetTick();
	if (HAL_GetTick() - key_time < 200)
		return;
	key_time = HAL_GetTick();

	int8_t row_number = -1;
	int8_t column_number = -1;

	if (GPIO_Pin == GPIO_PIN_0) {
		// blue_button_pressed = 1;
		// return;
	}

	for (uint8_t row = 0; row < 4; row++) // Loop through Rows
			{
		if (GPIO_Pin == Row_pins[row]) {
			row_number = row;
		}
	}

	HAL_GPIO_WritePin(Column_ports[0], Column_pins[0], 0);
	HAL_GPIO_WritePin(Column_ports[1], Column_pins[1], 0);
	HAL_GPIO_WritePin(Column_ports[2], Column_pins[2], 0);
	HAL_GPIO_WritePin(Column_ports[3], Column_pins[3], 0);

	for (uint8_t col = 0; col < 4; col++) // Loop through Columns
			{
		HAL_GPIO_WritePin(Column_ports[col], Column_pins[col], 1);
		if (HAL_GPIO_ReadPin(Row_ports[row_number], Row_pins[row_number])) {
			column_number = col;
		}
		HAL_GPIO_WritePin(Column_ports[col], Column_pins[col], 0);
	}

	HAL_GPIO_WritePin(Column_ports[0], Column_pins[0], 1);
	HAL_GPIO_WritePin(Column_ports[1], Column_pins[1], 1);
	HAL_GPIO_WritePin(Column_ports[2], Column_pins[2], 1);
	HAL_GPIO_WritePin(Column_ports[3], Column_pins[3], 1);

	if (row_number == -1 || column_number == -1) {
		return; // Reject invalid scan
	}
	const uint8_t button_number = row_number * 4 + column_number + 1;
	switch (button_number) {
	case 1:
		if (stage == 1) {
			stage = 2;

		} else if (stage == 2) {

			stage = 3;

		}
		break;
	case 2:
		pointer++;
		if (pointer > 2)
			pointer = 0;

		break;
	case 3: //charkhesh tank chap
		if (stage == 3) {

			rotate(&player1);


		}
		//code
		break;
	case 4: //charkhesh tank rast
		if(stage == 3){
			 rotate(&player2);
		}

		break;
	case 5:
		/* code */
		break;
	case 6:

		break;
	case 7://harkat tank chap
		if (stage == 3) {
			move(&player1);

		}
		break;
	case 8:
		if (stage == 3) {
			move(&player2);

		}
		break;
	case 9:
		/* code */
		break;
	case 10:
		/* code */
		break;
	case 11:
		/* code */
//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_8);
		break;
	case 12:
		/* code */
		break;
	case 13: //1
		/* code */
		break;
	case 14: //2
		/* code */
		break;
	case 15:
		/* code */
		break;
	case 16:
		stage--;
		/* code */
		break;

	default:
		break;
	}
}

int gameStarted = 0;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USB_PCD_Init();
  MX_RTC_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	//keypad
	LiquidCrystal(GPIOD, GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11,
	GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14);
	begin(20, 4);
	//createChar(4, start);
	createChar(1, lefttank);
	createChar(2, lowe_tank);
	createChar(3, righttank);
	createChar(4, upper_tank);
	//createChar(5, leftwall);
	//createChar(6, rightwall);
	createChar(5, heart);
	createChar(6, lucky);
	createChar(7, bullets);
	createChar(8, barrier);

//	createChar(9, leftwall_down);
//	createChar(30, rightwall_down);
	HAL_TIM_Base_Start_IT(&htim3);
//  	mytime.Hours = 1;
//  	   mytime.Minutes = 12;
//  	   mytime.Seconds = 2;
//  	   mydate.
	HAL_UART_Receive_IT(&huart1, rx_byte, 1);
//	HAL_RTC_SetTime(&hrtc, &myTime, RTC_FORMAT_BIN);
//	HAL_RTC_SetDate(&hrtc, &myDate, RTC_FORMAT_BIN);
	Change_Melody(super_mario_bros, ARRAY_LENGTH(super_mario_bros));
	HAL_GPIO_WritePin(Column_ports[0], Column_pins[0], 1);
	HAL_GPIO_WritePin(Column_ports[1], Column_pins[1], 1);
	HAL_GPIO_WritePin(Column_ports[2], Column_pins[2], 1);
	HAL_GPIO_WritePin(Column_ports[3], Column_pins[3], 1);

	srand(0);

////		while(1);
//	for (int i = 0; i < 4; i++) {
//		for (int j = 0; j < 20; j++) {
//			array[i][j] = 0;
//		}
//	}

//	char timeStr[10];
	PWM_Start();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		static uint32_t last = 0;
		if (HAL_GetTick() - last > 5) {
			Update_Melody();
			last = HAL_GetTick();
		}
		if (stage == 1 && pre2 != 1) {
			if (pre2 != 1) {
				clear();
				pre2 = 1;
			}
			melody_playing = 1;

			setCursor(5, 2);
			write(1);
			setCursor(5, 0);
			write(1);
			setCursor(15, 0);
			write(1);
			setCursor(15, 2);
			write(1);

			setCursor(5, 1);
			print("BATTLE TANK");
			HAL_Delay(2000);
		} else if (stage == 2) {
			if (pre2 != 2) {
				clear();
				pre2 = 2;
			} else if (pointer == 0) {
				setCursor(0, 2);
				print(" ");
				setCursor(0, 0);
				print("O");
			} else if (pointer == 1) {
				setCursor(0, 0);
				print(" ");
				setCursor(0, 1);
				print("O");
			} else if (pointer == 2) {
				setCursor(0, 1);
				print(" ");
				setCursor(0, 2);
				print("O");
			}
			setCursor(1, 0);
			print("1.About ");
			setCursor(1, 1);
			print("2.Setting ");
			setCursor(1, 2);
			print("3.Start ");
		} else if (stage == 3) {
			if (pre2 != 3) {
				clear();
				pre2 = 3;
			}

			if (pointer == 0) {
				static uint32_t lasttt = 0;
				if (HAL_GetTick() - lasttt > 5) {
					HAL_RTC_GetTime(&hrtc, &myTime, RTC_FORMAT_BIN);
					lasttt = HAL_GetTick();
				}
				char timestr[10];
				sprintf(timestr, "%d:%d:%d", myTime.Hours, myTime.Minutes,
						myTime.Seconds);
				setCursor(0, 2);
				print(timestr);
				HAL_RTC_GetDate(&hrtc, &myDate, RTC_FORMAT_BIN);
				setCursor(0, 0);
				print("Sogand Razavi");
				setCursor(0, 1);
				print("Yasamin Azizi");
				setCursor(0, 3);
				print(info[3]);
				print(info[4]);
				print(info[5]);

			} else if (pointer == 1) {
				//name
				setCursor(0, 0);
				print(info[6]);
				setCursor(11, 0);
				print(info[7]);
				//health
				setCursor(0, 1);
				print(info[8]);

				//bullet
				setCursor(0, 2);
				print(info[9]);

				//on-off
				setCursor(0, 4);
				print(info[10]);

			} else if (pointer == 2) {
				melody_playing = 0;
				PWM_Change_Tone(0, 0);
				if (!gameStarted) {

					start_game();
					gameStarted = 1;

				}
//				write(4);
//				write(6);
//				write(5);
//				write(7);
				for (int i = 0; i < 4; i++) {
					for (int j = 0; j < 20; j++) {
						setCursor(j, i);
						write(array[i][j]);
					}
				}

			}
		}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 47;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, CS_I2C_SPI_Pin|LD4_Pin|LD3_Pin|LD5_Pin
                          |LD7_Pin|LD9_Pin|LD10_Pin|LD8_Pin
                          |LD6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pins : CS_I2C_SPI_Pin LD4_Pin LD3_Pin LD5_Pin
                           LD7_Pin LD9_Pin LD10_Pin LD8_Pin
                           LD6_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin|LD4_Pin|LD3_Pin|LD5_Pin
                          |LD7_Pin|LD9_Pin|LD10_Pin|LD8_Pin
                          |LD6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : MEMS_INT3_Pin MEMS_INT4_Pin */
  GPIO_InitStruct.Pin = MEMS_INT3_Pin|MEMS_INT4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC1 PC2 PC3
                           PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 PD11
                           PD12 PD13 PD14 PD4
                           PD5 PD6 PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD0 PD1 PD2 PD3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_TSC_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI2_TSC_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
