#include "stm32l476xx.h"

extern void max7219_init();
extern void MAX7219Send(unsigned char address,unsigned char data);


GPIO_TypeDef* GPIO[16] = {[0xA]=GPIOA, [0xB]=GPIOB, [0xC]=GPIOC};
const unsigned int X[4] = {0xA5, 0xA6, 0xA7, 0xB6};
const unsigned int Y[4] = {0xC7, 0xA9, 0xA8, 0xBA};

int PWM_num = 0;
int dthreshold=0x1FFF; //debounce threshold
int bthreshold=0x4FFF;
int gthreshold=0X6FFF;
int rthreshold=0X7FFF;
int score_A = 0,score_B = 0;
int round = 0; //0 for A, 1 for B
int out = 0;



void set_moder(int addr, int mode) { // mode: 0 input, 1 output , 2 alternate
	int x = addr >> 4, k = addr & 0xF;
	RCC->AHB2ENR |= 1<<(x-10);
	GPIO[x]->MODER &= ~(3 << (2*k));
	GPIO[x]->MODER |= (mode << (2*k));

	if (mode == 0) {
		GPIO[x]->PUPDR &= ~(3 << (2*k));
		GPIO[x]->PUPDR |= (2 << (2*k));
	}
}

void GPIO_init(){
	RCC->AHB2ENR = RCC->AHB2ENR| RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN |RCC_AHB2ENR_GPIOCEN ; //enable port A B

	//button and led
	set_moder(0xA0,1);//PA0 as blue out
	set_moder(0xA1,1);//PA1 as green out
	set_moder(0xA8,1);//PA8 as red out
	set_moder(0xA9,0);//PA9 as button input

	//PC0 1 as # of out led
	set_moder(0xC0,1);//1 out
	set_moder(0xC1,1);//2 out
	//pull-down output
	GPIOC->PUPDR = (GPIOC->PUPDR&0xFFFFFFF0)|0xA;

	//PA6 7 10 as 7-segment input
	//set_moder(0xA,1);
	set_moder(0xA6,1);//PA6 as output
	set_moder(0xA7,1);//PA7 as output
	set_moder(0xAA,1);//PA10 as output

	//motor initial
	set_moder(0XB0,1); //PB0 as motor in 1
	set_moder(0XB3,1); //PB1 as motor in 2
	//set PA0,1,8 as Pull-down output
	GPIOA->PUPDR=(GPIOA->PUPDR&0xFFFCFFF0)|0x2000A;
	GPIOB->ODR = (GPIOB->ODR&0xFFFFFF7F)|(1<<7); //motor enable
	GPIOB->ODR = (GPIOB->ODR&0xFFFFFFF6)|(1<<3); //motor reverse rotate

	//crash button initial
	set_moder(0xB4,0); //PB4 as crash input
	set_moder(0xB5,0); //PB5 as crash2 input
	//test
	set_moder(0xB6,1);
	GPIOB->ODR = (GPIOB->ODR&0xFFFFFFBF);
	//
	GPIOB->PUPDR=(GPIOB->PUPDR&0xFFFFFCFF)|0x500; //PB4 as pull down input

}

void EXTI_config(){
	RCC-> APB2ENR |=  RCC_APB2ENR_SYSCFGEN; //open syscfg
	SYSCFG -> EXTICR[1] |= SYSCFG_EXTICR2_EXTI4_PB; //Interrupt source from PB4
	SYSCFG -> EXTICR[1] |= SYSCFG_EXTICR2_EXTI5_PB; //Interrupt source from PB5
	EXTI -> IMR1 |= EXTI_IMR1_IM4;
	EXTI -> IMR1 |= EXTI_IMR1_IM5;

	EXTI -> FTSR1 |= EXTI_FTSR1_FT4;
	EXTI -> FTSR1 |= EXTI_FTSR1_FT5;
	return;

}
void NVIC_config() {
  NVIC_EnableIRQ(EXTI4_IRQn); //enable exti4 interrupt in nvic
  NVIC_SetPriority(EXTI4_IRQn, 0);
  NVIC_EnableIRQ(EXTI9_5_IRQn); //enable exti5 6 7 int nvic
  NVIC_SetPriority(EXTI9_5_IRQn, 0);
  return ;
}
void EXTI4_IRQHandler(void) {

	GPIOB->ODR = (GPIOB->ODR&0xFFFFFFBF)|(1<<6);
	if(round == 0){ //Player A scored
		score_A++;
	}
	else{
		score_B++;
	}
	display(score_A,score_B);
	for(int i=0;i<200000;i++){
		;
	}
	if (EXTI->PR1 & EXTI_PR1_PIF4){
		  EXTI->PR1 |= EXTI_PR1_PIF4;
	}
}
void EXTI9_5_IRQHandler(void) {
	GPIOB->ODR = (GPIOB->ODR&0xFFFFFFBF);
	if(++out == 3){
		out = 0;
		round = (round == 1)?0:1;
	}
	display_out(out);
	for(int i=0;i<200000;i++){
		;
	}
	if (EXTI->PR1 & EXTI_PR1_PIF5) {
	  EXTI->PR1 |= EXTI_PR1_PIF5;
	}

}

void GPIO_init_AF(){
//TODO: Initial GPIO pin as alternate function for buzzer. You can choose to use C or assembly to finish this function.
	GPIOA->MODER = (GPIOA->MODER & 0xfffff3ff) | 0x00000800; //set PA5 as AF mode
	GPIOA->OTYPER =(GPIOA->OTYPER& 0XFFFFFFDF);
	GPIOA->AFR[0] |= 1<<20; //set PA5 as AF1, which is TIM2_CH1
}




void Timer_init(){
//TODO: Initialize timer
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;//enable timer2
	TIM2->PSC = (uint32_t)4;//cnt would be increased by 1 every clk cycle.4MHz/100=40KHz
	TIM2->ARR = (uint32_t)100;//Reload value.
	TIM2->CCR1 = (uint32_t)PWM_num; // PWM_num is between 10-90
	TIM2->CCER |= TIM_CCER_CC1E; //Capture/Compare 1 output enable
	TIM2->CCMR1 = TIM2->CCMR1 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; //set mode as PWM mode 1. In upcounting, channel 1 is active as long as TIMx_CNT<TIMx_CCR1 else inactive.
	TIM2->EGR = TIM_EGR_UG;//Reinitialize the counter. CNT takes the auto-reload value.
	TIM2->CR1 |= TIM_CR1_CEN;//start timer
}
//void PWM_channel_init(){
//TODO: Initialize timer PWM channel
//}

void display(int A,int B){
	int digit[8] = {0};
	int to_send[8] = {0};
	int max_digit_A = 7,max_digit_B = 0;
	for(int team=0;team<2;team++){
		for(int i=0;i<4;i++){
			if(team == 0){
				digit[i] = B % 10;
				if(digit[i] != 0){
					max_digit_B = i;
				}
				B /= 10;
				if(i == 3 && B > 0){
					max_digit_B = 3;
					for(int j=0;j<4;j++) digit[j]=9;
				}
			}
			else{
				digit[i+4] = A % 10;
				if(digit[i+4] != 0){
					max_digit_A = 7-i;
				}
				A /= 10;
				if(i == 3 && A > 0){
					max_digit_A = 4;
					for(int j=4;j<8;j++) digit[j]=9;
				}

			}
		}
	}
	for(int k=0;k<4;k++){
		if(digit[k] == 0 && k > max_digit_B){
			to_send[k] = 15;
		}
		else{
			to_send[k] = digit[k];
		}
		if(max_digit_A > k+4){
			to_send[k+4] = 15;
		}
		else{
			to_send[k+4] = digit[k+4-(max_digit_A-4)];
		}
	}
	for(int i=0;i<8;i++){
		MAX7219Send(i+1,to_send[i]);
	}
}

void display_out(int num){
	if(num == 0){
		GPIOC->ODR = (GPIOC->ODR&0xFFFC);
	}
	else if(num == 1){
		GPIOC->ODR = (GPIOC->ODR&0xFFFC)|0x1;
	}
	else if(num == 2){
		GPIOC->ODR = (GPIOC->ODR&0xFFFC)|0x3;
	}
}
int main(){
	GPIO_init();
	GPIO_init_AF();
	NVIC_config();
	EXTI_config();
	Timer_init();
	max7219_init();
	display(score_A,score_B);

	while(1){
		int cnt=0;
		int resultflag=0;


		while(1){
			int button = GPIOA->IDR &(1<<9);
			if(button==512){//button push
				cnt++;
				if(cnt<dthreshold){
					/*GPIOA->ODR = (GPIOA->ODR&0xFFFFFEFF);
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFE);
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFD);*/
					PWM_num=30;
				}
				else if(cnt>=dthreshold && cnt<bthreshold){
					resultflag=1;
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFE)|(1<<0);//turn on blue
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFD); //turn off green
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFEFF); //turn off red

					PWM_num=40;



				}
				else if(cnt>=bthreshold && cnt<gthreshold){
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFE)|(1<<0);
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFD)|(1<<1);//turn on green
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFEFF);
					PWM_num=50;

				}
				else if(cnt>=gthreshold && cnt<rthreshold){
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFE)|(1<<0);
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFD)|(1<<1);
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFEFF)|(1<<8);//turn on red

					PWM_num=70;

				}
				else if(cnt>=rthreshold){
					cnt=0;
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFEFF);
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFE);
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFD);
					PWM_num=30;
					/*GPIOA->ODR = (GPIOA->ODR&0xFFFFFEFF);
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFE);
					GPIOA->ODR = (GPIOA->ODR&0xFFFFFFFD);*/
				}

			}
			else{
				cnt=0;
				if(resultflag==1)break;


			}


		}
		if(resultflag==1){
				resultflag=0;
				cnt=0;

				TIM2->CCR1 = (uint32_t)PWM_num;

		}




	}
}
