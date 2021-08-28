/* 	File: 		main.c
* 	Author: 	Jingyi Yan
*/

/* system imports */
#include <mbed.h>
#include <math.h>
#include <USBSerial.h>
#include <stdio.h>
/* user imports */
#include "LIS3DSH.h"


/* USBSerial library for serial terminal */
USBSerial serial(0x1f00,0x2012,0x0001,false);

/* LIS3DSH Library for accelerometer  - using SPI*/
LIS3DSH acc(PA_7, SPI_MISO, SPI_SCK, PE_3);

/* LED output */
DigitalOut ledOut(LED1);
DigitalOut rLED(PD_14);   // on board red LED, DO
DigitalOut bLED(PD_15);   // on board blue LED. DO
DigitalOut gLED(PD_12);   // on board green LED, DO
DigitalOut oLED(PD_13);   // on board orange LED, DO
double delay = 0.2; 
DigitalIn Mypin(USER_BUTTON);

const uint8_t N = 100; 					// filter length
float ringbuf_z[N];
float ringbuf_x[N];
float ringbuf_y[N];
float ringbuf_angle[N];
bool ismenu = true;	
int counter_squat = 0;
int counter_situp = 0;
int counter_pushup = 0;
int counter_jumpjack = 0;
const float PI = 3.1415926;					// sample buffer 

/* state type definition */
 typedef enum state_t {
 	pushup,
 	situp,
	squats,
	jumpjack,
	relax
 } state_t;


/* read data from the accelerometer and do some processing */
void read_data(double *ax,double *ay,double *az){
	int16_t X, Y;							// not really used
	int16_t zAccel = 0; 	
	acc.ReadData(&X, &Y, &zAccel);
	*ax = (float)X/17694.0;
	*ay = (float)Y/17694.0;
	*az = (float)zAccel/17694.0;
}

void detect(){
	
		double ax,ay,az;

		while(1){
		/* check detection of the accelerometer */
		if(acc.Detect() != 1) {
			serial.printf("Could not detect Accelerometer\n\r");
			wait_ms(200);
		}else{
		/* add all samples */
		serial.printf("Please Start");
			for (uint8_t i = 0; i < N; i++) {
				read_data(&ax,&ay,&az);
				ringbuf_z[i] = az;
				ringbuf_x[i] = ax;
				ringbuf_y[i] = ay;
				if(az>1){
					az = 1;
				}
				ringbuf_angle[i] = 180*acos(az)/PI ;
				serial.printf("-");
				wait_ms(100);
			}
		break;

		}

	}
	
}

void print_result(){
	
	float sum_z = 0;
	float sum_x = 0;
	float sum_y = 0;
	bool isZGreatOne = false;
	bool isXGreatOne = false;
	bool isXSmallZero = false;
	bool isXGreatOnedotFif = false;
	bool xGreat = false;
	bool angleGreat = false;
	bool zGreat = false;

for (uint8_t i = 0; i < N; i++) {
	if(ringbuf_z[i]>1){
		isZGreatOne = true;
	}
	if(ringbuf_x[i]>1){
		isXGreatOne = true;
	}
	if(ringbuf_x[i]<0){
		isXSmallZero = true;
	}
	if(ringbuf_x[i]>1.15){
		isXGreatOnedotFif = true;
	}
	
	sum_z += ringbuf_z[i];
	sum_x += ringbuf_x[i];
	sum_y += ringbuf_y[i];	
	}

	float z_average = sum_z/(float)N;
	

	if(z_average > 1){
		z_average = 1;
	}

	// calculate the angle 
	float angle = 180*acos(z_average)/PI;
	serial.printf("Current angle: %.2f degrees \r\n", angle);
	state_t state = relax;

	// judge which state you are in	
	if (angle > 130) {
			state = pushup;
	}else if(isZGreatOne){
			state = situp;
	}else if(isXGreatOne&&isXSmallZero){
			state = jumpjack;
	}else if(isXGreatOnedotFif){
			state = squats;
	}else{
			state = relax;
	}

	switch (state) {
		case pushup: // push up
			serial.printf("Now you are pushing up\r\n");
			// count the number of times of push up
			for(uint8_t i = 0; i < N; i++){
				if(ringbuf_z[i]>-0.7){
					zGreat = true;
				}
				if(ringbuf_z[i]<-1.1&&zGreat){
					counter_pushup=counter_pushup+1;
					zGreat = false;
				}
			}
		break;

		case situp:	// sitting up
			serial.printf("Now you are sit up\r\n");
			// count the number of times of sit up
			for(uint8_t i = 0; i < N; i++){
				if(ringbuf_angle[i]>100){
					angleGreat = true;
				}
				if(ringbuf_angle[i]<30&&angleGreat){
					counter_situp=counter_situp+1;
					angleGreat = false;
				}
			}


		break;
		
		case squats: // squats
			serial.printf("Now you are squats\r\n");
			// count the number of times of squats
			for(uint8_t i = 0; i < N; i++){
				if(ringbuf_x[i]>1.1){
					xGreat = true;
				}
				if(ringbuf_x[i]<0.8&&xGreat){
					counter_squat=counter_squat+1;
					xGreat = false;
				}
			}

		break;

		case jumpjack: // jumping jack
			serial.printf("Now you are jumpjack\r\n");
			// count the number of times of jump jack
			for(uint8_t i = 0; i < N; i++){
				if(ringbuf_x[i]>1.5){
					xGreat = true;
				}
				if(ringbuf_x[i]<0.5&&xGreat){
					counter_jumpjack=counter_jumpjack+1;
					xGreat = false;
				}
			}

			break;

		case relax: // relax state
			serial.printf("Now you are doing nothing\n\r");
			break; 

			default:
				printf("you almost did nothing\n\r");
		}

}

void menu(){
      serial.printf("Start in 5 seconds\r\n");
      wait_ms(100);
}

// blinkLED() to show all the exercises are done
void blinkLED(){
	bLED = 1;
	rLED = 1;
    gLED = 1;
	oLED = 1;
}

// check whether you have finished each exercise 5 times
bool check_status(){
	if(counter_jumpjack>=5){
		bLED = 1;
	}
	if(counter_pushup>=5){
		oLED = 1;
	}
	if(counter_squat>=5){
		rLED = 1;
	}
	if(counter_situp>=5){
		gLED = 1;
	}
	if(counter_situp>=5&&counter_pushup>=5&&counter_jumpjack>=5&&counter_squat>=5){
		return true;
	}
	return false;
}

//set the counter and LED status to zero
void setZero(){
	counter_pushup = 0;
	counter_jumpjack = 0;
	counter_situp = 0;
	counter_squat = 0;
	bLED = 0;
	rLED = 0;
    gLED = 0;
	oLED = 0;
}

int main() {

    const uint8_t N = 20; 					// filter length

	
while(1){
	while(!check_status()){
	// press the user button to get started
	if(Mypin == 1){
		if(ismenu){
			menu();
		}
		ismenu = false;
		wait_ms(5000);
		detect();
		print_result();
		// print current summary of your exercise
		serial.printf("squat %d, pushup %d,jumpjack %d,situp %d\r\n",counter_squat,counter_pushup,counter_jumpjack,counter_situp);
		}
		
		ismenu = true;
	}
	blinkLED();
	setZero();
	serial.printf("You are all set \r\n");
}
}