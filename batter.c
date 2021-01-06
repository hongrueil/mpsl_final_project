#include "stm32l476xx.h"

int Table[4][4]={{1,2,3,10},{4,5,6,11},{7,8,9,12},{15,0,14,13}};

int PWM_num = 5;
int angle = 0;
int flag;

int angle_to_pwm(int angle){
	return (angle/9)+5;
}

void GPIO_init(){
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; //enable port A
}

void GPIO_init_AF(){
//TODO: Initial GPIO pin as alternate function for buzzer. You can choose to use C or assembly to finish this function.
	GPIOA->MODER = (GPIOA->MODER & 0xfffff3ff) | 0x00000800; //set PA5 as AF mode
	GPIOA->AFR[0] |= 1<<20; //set PA5 as AF1, which is TIM2_CH1
}

void button_init(){
	// SET button gpio INPUT/OUTPUT //
	//Set PA0 as output,PA1 as input
	GPIOA->MODER= (GPIOA->MODER&0xFFFFFFF0)|0x1;
	//set PA0 is Pull-up output,PA1 as pull-down input
	GPIOA->PUPDR=(GPIOA->PUPDR&0xFFFFFFF0)|0x9;
	//Set PA0,1 as medium speed mode
	GPIOA->OSPEEDR=GPIOA->OSPEEDR|0x5;
	//Set PA0 as high
	GPIOA->ODR=GPIOA->ODR|0x1;
}


void Timer_init(){
//TODO: Initialize timer
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;//enable timer2
	TIM2->PSC = (uint32_t)399;//cnt would be increased by 1 every clk cycle.
	TIM2->ARR = (uint32_t)200;//Reload value.
	TIM2->CCR1 = (uint32_t)angle_to_pwm(0); // PWM_num is between 10-90
	TIM2->CCER |= TIM_CCER_CC1E; //Capture/Compare 1 output enable
	TIM2->CCMR1 = TIM2->CCMR1 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; //set mode as PWM mode. In upcounting, channel 1 is active as long as TIMx_CNT<TIMx_CCR1 else inactive.
	TIM2->EGR = TIM_EGR_UG;//Reinitialize the counter. CNT takes the auto-reload value.
	TIM2->CR1 |= TIM_CR1_CEN;//start timer
}
//void PWM_channel_init(){
//TODO: Initialize timer PWM channel
//}

void wait(int time){
	while(time--);
}

void swing(){
	TIM2->CCR1 = (uint32_t)angle_to_pwm(180);
	wait(120000);
	TIM2->CCR1 = (uint32_t)angle_to_pwm(0);
	wait(120000);
}

void forward(){
	if(angle < 180){
		angle++;
	}
	TIM2->CCR1 = (uint32_t)angle_to_pwm(angle);
	wait(800);
}

void back(){
	if(angle > 0){
		angle--;
	}
	TIM2->CCR1 = (uint32_t)angle_to_pwm(angle);
	wait(800);
}

int main(){
	GPIO_init();
	GPIO_init_AF();
	button_init();
	Timer_init();

	//TODO: Scan the keypad and use PWM to send the corresponding frequency square wave to the buzzer.

	while(1){/*
		if(angle == 180) flag = 0;
		else if(angle == 0) flag = 1;
		TIM2->CCR1 = (uint32_t)angle_to_pwm(angle);
		wait(1000);
		if(flag) angle++;
		else angle--;*/
		int push = (GPIOA->IDR & 0x2)>>1;
		if(push)
			forward();
		else
			back();

	}
}
