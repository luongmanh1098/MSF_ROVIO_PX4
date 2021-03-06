/*

Copyright (c) 2011, Ascending Technologies GmbH
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

 */

/*
*	Camera Pan Tilt Unit (PTU) control
*/

#include "main.h"
#include "system.h"
#include "pelican_ptu.h"
#include "system.h"
#include "LPC214x.h"

struct CAMERA_PTU CAMERA_ptu;
struct CAMERA_COMMANDS CAMERA_Commands;

void PTU_init(void)
{
#ifdef HUMMINGBIRD_ROLL_SERVO
#ifndef HUMMINGBIRD_ROLL_SERVO_ON_SSEL0
	PINSEL0&=~0x01;
	PINSEL0|=0x02;
#else
	PINSEL0|=0x8000;
	PINSEL0&=~0x4000;
#endif
#endif

#ifdef PELICAN_PTU

    CAMERA_ptu.servo_pitch_offset=61500;
	CAMERA_ptu.servo_pitch_scale=54853;
	CAMERA_ptu.servo_pitch_min=46000;
	CAMERA_ptu.servo_pitch_max=128000;

	CAMERA_ptu.servo_roll_offset=82000;
	CAMERA_ptu.servo_roll_scale=220000;
	CAMERA_ptu.servo_roll_min=46000;
	CAMERA_ptu.servo_roll_max=128000;

#endif
}


void PTU_update(void)
{
	static int ptu_cnt=0;
	if(++ptu_cnt>9)	//generate 100Hz
	{
		ptu_cnt=0;
    	int angle_pitch, angle_roll;
#ifdef CAMMOUNT_XCONFIG	//rotate pitch/roll tiltcompensation for 45?
#ifndef CAM_FACING_FRONT_RIGHT
    angle_pitch=IMU_CalcData.angle_nick*707/1000+IMU_CalcData.angle_roll*707/1000;
    angle_roll=IMU_CalcData.angle_roll*707/1000-IMU_CalcData.angle_nick*707/1000;
#else
    angle_roll=IMU_CalcData.angle_nick*707/1000+IMU_CalcData.angle_roll*707/1000;
    angle_pitch=-IMU_CalcData.angle_roll*707/1000+IMU_CalcData.angle_nick*707/1000;
#endif
#else
    angle_pitch=IMU_CalcData.angle_nick;
    angle_roll=IMU_CalcData.angle_roll;
#endif
#ifndef	HUMMINGBIRD_ROLL_SERVO
		if(CAMERA_Commands.status&0x02)	//no tilt compensation
		{
			SERVO_move_analog((CAMERA_OFFSET_HUMMINGBIRD+CAMERA_Commands.desired_angle_pitch)*HUMMINGBIRD_SERVO_DIRECTION_PITCH);
		}
		else
		{
			int t=0;	//to overcome compiler optimization problem
			SERVO_move_analog(t+(CAMERA_OFFSET_HUMMINGBIRD+CAMERA_Commands.desired_angle_pitch+IMU_CalcData.angle_nick)*HUMMINGBIRD_SERVO_DIRECTION_PITCH);
		}
#else
		static int cam_angle_pitch=0;
#ifdef SET_CAMERA_ANGLE_INCREMENTAL
		if(LL_1khz_attitude_data.RC_data[4]>192) cam_angle_pitch+=200;
		else if(LL_1khz_attitude_data.RC_data[4]<64) cam_angle_pitch-=200;
		if(cam_angle_pitch>55000) cam_angle_pitch=55000;
		if(cam_angle_pitch<-55000) cam_angle_pitch=-55000;
#else
		cam_angle_pitch=CAMERA_Commands.desired_angle_pitch;
#ifdef APTINA
		if(cam_angle_pitch<-90000) cam_angle_pitch=-90000;
		else if(cam_angle_pitch>0) cam_angle_pitch=0;
#endif
#endif

		if(CAMERA_Commands.status&0x02)	//no tilt compensation
		{
			SERVO_pitch_move((CAMERA_OFFSET_HUMMINGBIRD_PITCH+cam_angle_pitch)*HUMMINGBIRD_SERVO_DIRECTION_PITCH);
			SERVO_roll_move((CAMERA_OFFSET_HUMMINGBIRD_ROLL+CAMERA_Commands.desired_angle_roll)*HUMMINGBIRD_SERVO_DIRECTION_ROLL);
		}
		else
		{
			SERVO_pitch_move((CAMERA_OFFSET_HUMMINGBIRD_PITCH+cam_angle_pitch+angle_pitch)*HUMMINGBIRD_SERVO_DIRECTION_PITCH);
			SERVO_roll_move((CAMERA_OFFSET_HUMMINGBIRD_ROLL+CAMERA_Commands.desired_angle_roll+angle_roll)*HUMMINGBIRD_SERVO_DIRECTION_ROLL);
		}
#endif
	}
}


void SERVO_pitch_move (int angle)
{
    unsigned int value;
    value=CAMERA_ptu.servo_pitch_offset+(angle/10)*CAMERA_ptu.servo_pitch_scale/9000;	//9000

    if(value>CAMERA_ptu.servo_pitch_max) value=CAMERA_ptu.servo_pitch_max;
    else if(value<CAMERA_ptu.servo_pitch_min) value=CAMERA_ptu.servo_pitch_min;

    PWMMR5 = value;
    PWMLER = LER5_EN|LER1_EN|LER2_EN;
}

void SERVO_roll_move (int angle)
{
    int value;
    value=CAMERA_ptu.servo_roll_offset+(angle/10)*CAMERA_ptu.servo_roll_scale/9000;	//9000

    if(value>CAMERA_ptu.servo_roll_max) value=CAMERA_ptu.servo_roll_max;
    else if(value<CAMERA_ptu.servo_roll_min) value=CAMERA_ptu.servo_roll_min;

#ifdef HUMMINGBIRD_ROLL_SERVO_ON_SSEL0
    PWMMR2 = value;
#else
    PWMMR1 = value;
#endif
    PWMLER = LER5_EN|LER1_EN|LER2_EN;
}


