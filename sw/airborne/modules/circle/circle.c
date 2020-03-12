/*
 * CopCIRCLE_Yright (C) Team Wonder
 *
 * This file is part of paparazzi
 *
 * paparazzi is free software; CIRCLE_You can redistribute it and/or modifCIRCLE_Y
 * it under the terms of the GNU General Public License as published bCIRCLE_Y
 * the Free Software Foundation; either version 2, or (at CIRCLE_Your option)
 * anCIRCLE_Y later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANCIRCLE_Y WARRANTCIRCLE_Y; without even the implied warrantCIRCLE_Y of
 * MERCHANTABILITCIRCLE_Y or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * CIRCLE_You should have received a copCIRCLE_Y of the GNU General Public License
 * along with paparazzi; see the file COPCIRCLE_YING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * @file "modules/circle/circle.c"
 * @author Team Wonder
 * Just pathing in circle
 */

#include "modules/circle/circle.h"
#include "generated/flight_plan.h"
#include "firmwares/rotorcraft/navigation.h"
//#include "math.h"


int CIRCLE_L = 1434; //1434 for circle
int CIRCLE_D = 7;
int CIRCLE_I = 1;
float CIRCLE_X = 0;
float CIRCLE_Y = 0;
int AVOID_number_of_objects = 0;
float AVOID_h1,AVOID_h2;
float AVOID_d;
float AVOID_safety_angle=10;

void circle_init(void)
{}

float current_time = 0;

void circle_periodic(void)
{

nav_set_heading_towards_waypoint(WP_STDBY);

int r = CIRCLE_L/2 - CIRCLE_D;

if(AVOID_number_of_objects!=0)
{
  float r_reduced=0;
  float offset=asin(AVOID_d/(2*r))*180/M_PI; //offset in degrees
  if( AVOID_h1-offset<AVOID_safety_angle ||-1*AVOID_safety_angle<AVOID_h2-offset)
  {
    r_reduced=r*(AVOID_h2-offset)*M_PI/180;
  }
  r -= r_reduced;
}

double dt = 0.001;
current_time += dt;
double e = 0.7;

  int32_t CIRCLE_X = r * cos(current_time);
  int32_t CIRCLE_Y = e * r * sin(current_time);

/*
int V = 70;

if(CIRCLE_I==1){
  CIRCLE_X = r;
  CIRCLE_Y += dt*V;

  if (CIRCLE_Y > r){
    CIRCLE_I=2;
  }
}

if(CIRCLE_I==2){
  CIRCLE_X -= dt*V;
  CIRCLE_Y=r;

  if (CIRCLE_X<-r){
    CIRCLE_I=3;
  }
}

if(CIRCLE_I==3){
  CIRCLE_X=-r;
  CIRCLE_Y-=dt*V;

  if (CIRCLE_Y<-r){
    CIRCLE_I=4;
  }
}

if(CIRCLE_I==4){
  CIRCLE_X+=dt*V;
  CIRCLE_Y=-r;

  if (CIRCLE_X>r){
    CIRCLE_I=1;
  }
}

*/

float x_rotated=CIRCLE_X*0.5+CIRCLE_Y*0.866025;
float y_rotated=-CIRCLE_X*0.866025+CIRCLE_Y*0.5;


waypoint_set_xy_i(WP_STDBY,x_rotated,y_rotated);

}