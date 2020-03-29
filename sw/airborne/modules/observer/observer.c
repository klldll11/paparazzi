/*
 * Copyright (C) Team Wonder
 *
 * This file is part of paparazzi
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * @file "modules/TRAJECTORY/TRAJECTORY.h"
 * @author Team Wonder
 * Just pathing in TRAJECTORY
 */


#include "modules/observer/observer.h"


#ifndef OPENCVDEMO_FPS
#define OPENCVDEMO_FPS 0
#endif

#ifndef COLORFILTER_CAMERA
#define COLORFILTER_CAMERA front_camera
#endif

// Resized image size

// #define scale_h 3
// #define scale_w 3

// #define new_h 173 // 520/scale_h
// #define new_w 80  // 240/scale_w

// #define scale_h 2
// #define scale_w 2

// #define new_h 260 // 520/scale_h
// #define new_w 120 // 240/scale_w

// #define scale_h 4
// #define scale_w 4

// #define new_h 130 // 520/scale_h
// #define new_w 60  // 240/scale_w

#define scale_h 1
#define scale_w 1

#define new_h 520 // 520/scale_h
#define new_w 240  // 240/scale_w



// Variables declaration
  // Drone state
  float x_loc;
  float y_loc;
  float head;
  float vx_loc;
  float vy_loc;
  float v_tot;
  float rot_loc;

  // color filters        y_m  y_M  u_m  u_M  v_m  v_M
  uint8_t orange_cf[6] = { 26, 164,  45, 130, 160, 192};
  uint8_t green_cf[6]  = { 70, 230,   0, 120,   0, 125};
  uint8_t blue_cf[6]   = { 67, 255, 120, 255,   0, 125};
  uint8_t black_cf[6]  = {116, 140, 116, 140,   0,  120};

  // orange mask
  uint8_t mask_o[new_h][new_w];
  // green mask
  uint8_t mask_g[new_h][new_w];
  // edge mask
  uint8_t mask_e[new_h][new_w];

  // limits on green mask object detection:
  // do not check if outside these boundaries
  const float top_x_green = 3.5;
  const float top_y_green = 3.5;

  const float head_x_t_green = 2.2;
  const float head_x_b_green = 1;
  const float head_y_t_green = 0.6;
  const float head_y_b_green = 2.5; // capo is on 2.618

  // bool to check (if close to the edge, don't look)
  bool check_green;
  bool started_edge;


  // floor of green mask
  uint16_t floors[new_h];

  // 3x3 kernel
  uint8_t kernel[3][3] = {{0, 0, 0},
                          {1, 0, -1},
                          {0, 0, 0}};

  // const int8_t big_kernel[5][5] = {{1, 1, 0, -1, -1},
  //                                 {2, 2, 0, -2, -2},
  //                                 {3, 3, 0, -3, -3},
  //                                 {2, 2, 0, -2, -2},
  //                                 {1, 1, 0, -1, -1}};


  const int8_t big_kernel[5][5] = {{1, 0, -1},
                                   {3, 0, -3},
                                   {1, 0, -1}};


  // big kernel length
  // const uint8_t ker_l = 5;
  // const uint8_t ker_l2 = 2;
  const uint8_t ker_l = 3;
  const uint8_t ker_l2 = 1;

  // const uint8_t sum_k = 36;
  const uint8_t sum_k = 10;

  // Blur size
  // const uint8_t blur_w = 21;
  // const uint8_t blur_w2 = 10;
  const uint8_t blur_w = 11;
  const uint8_t blur_w2 = 5;

  const uint8_t blur_h = 1;
  const uint8_t blur_h2 = 0;

  // detected poles (max 50 at a time) -> (left_px, right_px, distance)
    // poles 00-14 are used for orange
    // poles 15-29 are used for green
    // poles 30-49 are used for edge detector
  //  
  uint8_t idx_o = 0;
  uint8_t idx_g = 15;
  uint8_t idx_e = 30;

  const float px_dist_scale = 107.232;

  // 50 objects was assumed to be the maximum number of object to be at once 
  // in the cyberzoo
  uint16_t poles[50][2];           // left px, right px
  uint16_t poles_comb[50][3];      // left_px, right_px, type
                                    // types are: 0 = undefined; 1 = orange pole
  int16_t poles_w_inertia[50][4];  // left_px, right_px, type, times_seen
  float final_objs[50][4];         // left px, right px, dist, type
  uint8_t count_o;
  uint8_t count_g;
  uint8_t count_e;
  uint8_t count;
  uint8_t count_inertia;
  uint8_t final_count;
  uint8_t obj_merged[50];
  uint8_t idx_to_rm[50];
  uint8_t elem_to_rm;
  uint8_t edge_search_type;
  uint16_t px_change;

  uint16_t sums_o[new_h]; // cumulative sum
  uint16_t sums_e[new_h]; // cumulative sum
  uint16_t sums_e_smooth[new_h]; // smoothed cumulative sum

  // Distance threshold for same object [pixels]
  const uint8_t threshold = 30;
  const uint8_t associate_threshold = 70;

  struct image_t processed;
  struct image_t blurred;

  // Initialise the image transforms
  bool first_time = true;
//

////////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION
struct image_t *observer_func(struct image_t *img){

// nav_set_heading_towards_waypoint(WP_STDBY);

  if (img->type == IMAGE_YUV422) {

    // Take the time
    clock_t begin = clock();

    // Copy input image to processed, blurred
    if (first_time){
      // create_img(img, &processed);
      // create_img(img, &blurred);

      create_small_img(&processed);
      create_small_img(&blurred);

      first_time = false;
    }

    // Clean pole lists data
    for (uint16_t x = 0; x < 50; x++) {
      poles[x][0] = 0;
      poles[x][1] = 0;
      poles_comb[x][0] = 0;
      poles_comb[x][1] = 0;
      poles_comb[x][2] = 0;
      obj_merged[x] = 0;
      idx_to_rm[x] = 0;
      final_objs[x][0] = 0;
      final_objs[x][1] = 0;
      final_objs[x][2] = 0;
      final_objs[x][3] = 0;
    }

    // Downsize the input image
    downsize_img(img, &processed);

    // Get the drone's posititon and heading for object detector
    read_drone_state();

    // Filter poles (orange) and find objects from mask
    image_specfilt(&processed, &processed, orange_cf[0], orange_cf[1], orange_cf[2],
                    orange_cf[3], orange_cf[4], orange_cf[5], &mask_o);
    find_orange_objs();


    // Filter ground (green) and find objects from mask
    image_specfilt(&processed, &processed, green_cf[0], green_cf[1], 
                green_cf[2], green_cf[3], green_cf[4], green_cf[5], &mask_g);
    find_green_objs();

    // Filter blue color and remove noise in ground
    image_bgfilt(&processed, &processed, blue_cf[0], blue_cf[1], blue_cf[2],
                 blue_cf[3], blue_cf[4], blue_cf[5]);

    // Blur the processed image
    blur_big(&processed, &blurred);


    // copy2img(&processed, &blurred);
    // convolve_big(&processed, &processed);

    // Convolve the blurred image and find objects from edge mask
    convolve_big(&blurred, &processed);
    find_edge_objs(img);

    // Process all object measurements from the different sources, get rid of 
    // outliers, provide a continuous stream of objects detected and find 
    // distance to object
    combine_measurements();
    delete_outliers();
    find_distances();
    remap_to_original_img();

    // copy processed to img for output
    // copy2img(&blurred, img);

    // printf("Final measurements\n");
    // for (uint16_t x = 0; x < 10; x++){
    //   printf("[%.1lf, %.1lf, %.1lf] \n", final_objs[x][0], final_objs[x][1], final_objs[x][2]);
    // }

    // printf("\n");
    // printf("////////////////////////////////////////////////////////////\n");
    // copy2img(&processed, img);




    // copy2bigimg(&processed, img);

    // This is just to show the mask
    // for (uint16_t x=0; x<520; x++){
    //   sum = 0;
    //   for (uint16_t y=0; y<240; y++){
    //     printf("%d ",mask_r[x][y]);
    //     sum += mask_r[x][y];
    //   }
    //   printf("\t %d\n", sum);
    // }




    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("Time: %lf \n", time_spent);
  }

  return &processed;
  // return img;
  // return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Obtain the drone's state
void read_drone_state(void){
  // Get position and transform to own coordinate system
  // x defined looking from tables to cyberzoo
  // y defined to the right
  x_loc = - stateGetPositionEnu_f()->x*0.523599 
          + stateGetPositionEnu_f()->y*0.851965;
  y_loc =  stateGetPositionEnu_f()->x*0.851965 
         + stateGetPositionEnu_f()->y*0.523599;
  head = stateGetNedToBodyEulers_f()->psi + 0.523599;
  if (head > 3.141592){
    head -= 3.141592*2;
  }

  // Get velocity and transform to own coordinate system
  vx_loc = - stateGetSpeedEnu_f()->x*0.523599 
           + stateGetSpeedEnu_f()->y*0.851965;
  vy_loc =  stateGetSpeedEnu_f()->x*0.851965 
          + stateGetSpeedEnu_f()->y*0.523599;
  v_tot = sqrt(vx_loc*vx_loc + vy_loc*vy_loc);

  // Get angular rate
  rot_loc = stateGetBodyRates_f()->r;
}

// define shape of output images
void create_img(struct image_t *input, struct image_t *output){
  // Set the variables
  output->type = input->type;
  output->w = input->w;
  output->h = input->h;

  output->buf_size = sizeof(uint8_t) * 2 * input->w * input->h;
  output->buf = malloc(input->buf_size);
}

// define shape of downsized images
void create_small_img(struct image_t *output){
  // Set the variables
  output->type = IMAGE_YUV422;
  output->w = new_w;
  output->h = new_h;

  output->buf_size = sizeof(uint8_t) * 2 * new_w * new_h;
  output->buf = malloc(output->buf_size);
}  

// Decrease image resolution to 'new_h' and 'new_w'
void downsize_img(struct image_t *input, struct image_t *output){

  uint8_t *source = (uint8_t *)input->buf;
  uint8_t *dest = (uint8_t *)output->buf;

  output->ts = input->ts;

  for (uint16_t y = 0; y < new_h; y++) { // horizontal (columns) 173
    for (uint16_t x = 0; x < new_w; x += 2) { // vertical (rows) 80

      dest[0] = (source + 2*x*scale_w + 2*y*scale_h*input->w)[0];
      dest[1] = (source + 2*x*scale_w + 2*y*scale_h*input->w)[1];
      dest[2] = (source + 2*x*scale_w + 2*y*scale_h*input->w)[2];
      dest[3] = (source + 2*x*scale_w + 2*y*scale_h*input->w)[3];

      dest += 4;
    }
  }
}


// copy output of input image to output image
void copy2img(struct image_t *input, struct image_t *output){

  uint8_t *source = (uint8_t *)input->buf;
  uint8_t *dest = (uint8_t *)output->buf;

  for (uint16_t y = 0; y < (output->h); y++) { // horizontal (columns) 520
    for (uint16_t x = 0; x < (output->w); x += 2) { // vertical (rows) 240
      dest[0] = source[0];
      dest[1] = source[1];
      dest[2] = source[2];
      dest[3] = source[3];

      source += 4;
      dest += 4;
    }
  }
}

// Filter orange color and place black on the rest
void image_specfilt(struct image_t *input, struct image_t *output, uint8_t y_m, 
                     uint8_t y_M, uint8_t u_m, uint8_t u_M, uint8_t v_m, 
                     uint8_t v_M, uint8_t *mask){

  uint8_t *source = (uint8_t *)input->buf;
  uint8_t *dest = (uint8_t *)output->buf;

  // Go trough all the pixels
  for (uint16_t y = 0; y < input->h; y++) { // horizontal (columns) 520
    for (uint16_t x = 0; x < input->w; x += 2) { // vertical (rows) 240
      // Check if the color is inside the specified values
      if ( (source[1] >= y_m)
        && (source[1] <= y_M)
        && (source[0] >= u_m)
        && (source[0] <= u_M)
        && (source[2] >= v_m)
        && (source[2] <= v_M)
      ) {
        dest[0] = 128;  // U
        dest[1] = 255;  // Y
        dest[2] = 128;  // V
        dest[3] = 255;  // Y

        // Create binary mask with 1's for color filtered, and 0's for the rest
        *(mask+x+y*(output->w)) = 1;
        *(mask+x+1+y*(output->w)) = 1;

      } else {
        // dest[0] = 128;       // U // Grayscale
        // dest[1] = source[1]; // Y
        // dest[2] = 128;       // V
        // dest[3] = source[3]; // Y
          
        dest[0] = source[0];  // U // Color
        dest[1] = source[1];  // Y
        dest[2] = source[2];  // V
        dest[3] = source[3];  // Y


        *(mask+x+y*(output->w)) = 0;
        *(mask+x+1+y*(output->w)) = 0;
      }
      // Go to the next 2 pixels
      dest += 4;
      source += 4;
    }
  }
}

// Filter background color and place grayscale on the rest
void image_bgfilt(struct image_t *input, struct image_t *output, uint8_t y_m, 
                     uint8_t y_M, uint8_t u_m, uint8_t u_M, uint8_t v_m, 
                     uint8_t v_M){

  uint8_t *source = (uint8_t *)input->buf;
  uint8_t *dest = (uint8_t *)output->buf;

  // Copy the creation timestamp
  output->ts = input->ts;

  // Go trough all the pixels
  for (uint16_t y = 0; y < input->h; y++) {
    for (uint16_t x = 0; x < input->w; x += 2) {

      // Remove all ground below the floor to remove ground not classified as 
      // such
      if (x < floors[y]){
          source[1] = 255;
          source[3] = 255;
      }

      // Check if the color is inside the specified values
      if ( (source[1] >= y_m)
        && (source[1] <= y_M)
        && (source[0] >= u_m)
        && (source[0] <= u_M)
        && (source[2] >= v_m) 
        && (source[2] <= v_M)
      ) {
        dest[0] = 128;          // U
        dest[1] = 255;          // Y
        dest[2] = 128;          // V
        dest[3] = 255;          // Y
      } else {
        // UYVY
        // dest[0] = source[0]; // U  Color
        // dest[1] = source[1]; // Y
        // dest[2] = source[2]; // V
        // dest[3] = source[3]; // Y

        dest[0] = 128;          // U  Grayscale
        dest[1] = source[1]*2;  // Y
        dest[2] = 128;          // V
        dest[3] = source[3]*2;  // Y
      }

      // Go to the next 2 pixels
      dest += 4;
      source += 4;
    }
  }
}

// Blur an image by averaging over pixels next to it (uses blur_w and blur_h)
void blur_big(struct image_t *input, struct image_t *output){

  uint8_t *source = ((uint8_t *)input->buf);
  uint8_t *dest = ((uint8_t *)output->buf);

  // temp for average value
  int32_t val1;
  int32_t val3;

  // temp for limits on averaging pixels index
  int8_t bot_l1; // x
  int8_t top_l1;
  int8_t bot_l2; // y
  int8_t top_l2;

  // temp for number of pixels considered
  uint8_t sum_b;

  for (uint16_t y = 0; y < (input->h); y++) { // horizontal (columns) 520
    for (uint16_t x = 0; x < (input->w); x += 2) { // vertical (rows) 240
      val1 = 0;
      val3 = 0;

      // when pixel is close to any edge, limit the pixels to consider the 
      // average between to the picture limits
      if (x < 2*blur_w2){
        bot_l1 = -x/2;
        top_l1 = blur_w2+1;
      } else{
        if (x > (output->w)-2*blur_w2-2){
          bot_l1 = -blur_w2;
          top_l1 = (output->w - x)/2;
        } else {
          bot_l1 = -blur_w2;
          top_l1 = blur_w2+1;
        }
      }

      if (y < blur_h2){
        bot_l2 = -y;
        top_l2 = blur_h2+1;
      } else{
        if (y > (output->h)-blur_h2-1){
          bot_l2 = -blur_h2;
          top_l2 = (output->h - y);
        } else {
          bot_l2 = -blur_h2;
          top_l2 = blur_h2+1;
        }
      }

      sum_b = (top_l2 - bot_l2) * (top_l1 - bot_l1);

      // Compute average value of bounding pixels
      for (int8_t i = bot_l1; i < top_l1; i++){
        for (int8_t j = bot_l2; j < top_l2; j++){
          val1 += (source + 2*(x+2*i) + 2*(y+j)*output->w)[1];
          val3 += (source + 2*(x+2*i) + 2*(y+j)*output->w)[3];
        }
      }

      (dest + 2*x + 2*y*output->w)[0] = 128;
      (dest + 2*x + 2*y*output->w)[1] = abs(val1 / sum_b);
      (dest + 2*x + 2*y*output->w)[2] = 128;
      (dest + 2*x + 2*y*output->w)[3] = abs(val3 / sum_b);
    }
  }
}

// Convolve an image by multiplying pixels next to it with big_kernel
void convolve_big(struct image_t *input, struct image_t *output){

  uint8_t *source = ((uint8_t *)input->buf);
  uint8_t *dest = ((uint8_t *)output->buf);

  // temp for minimum and maximum values of the convolved image to remap to 
  // 0-255 range
  int16_t max_k1 = 0;
  int16_t min_k1 = 32000;
  int16_t max_k3 = 0;
  int16_t min_k3 = 32000;

  // temp for convolved pixel values
  int32_t val1;
  int32_t val3;

  for (uint16_t y = ker_l2; y < (input->h)-ker_l2; y++) { // hor (columns) 520
    for (uint16_t x = ker_l2; x < (input->w)-ker_l2; x += 2) { // ver (rows) 240
      val1 = 0;
      val3 = 0;

      // calculate convolution to highlight gradient between pixels
      for (int8_t i = -ker_l2; i < ker_l2+1; i++){
        for (int8_t j = -ker_l2; j < ker_l2+1; j++){
          val1 += (source + 2*(x+2*i) + 2*(y+j)*output->w)[1] 
                 * big_kernel[i+ker_l2][j+ker_l2];
          val3 += (source + 2*(x+2*i) + 2*(y+j)*output->w)[3] 
                 * big_kernel[i+ker_l2][j+ker_l2];
        }
      }

      (dest + 2*x + 2*y*output->w)[0] = 128;
      (dest + 2*x + 2*y*output->w)[1] = abs(val1/sum_k);
      (dest + 2*x + 2*y*output->w)[2] = 128;
      (dest + 2*x + 2*y*output->w)[3] = abs(val3/sum_k);

      // update max/min value
      if ((dest + 2*x + 2*y*output->w)[1] > max_k1) {
        max_k1 = (dest + 2*x + 2*y*output->w)[1];
      } if((dest + 2*x + 2*y*output->w)[1] < min_k1) {
        min_k1 = (dest + 2*x + 2*y*output->w)[1];
      } if ((dest + 2*x + 2*y*output->w)[3] > max_k3) {
        max_k3 = (dest + 2*x + 2*y*output->w)[3];
      } if ((dest + 2*x + 2*y*output->w)[3] < min_k3) {
        min_k3 = (dest + 2*x + 2*y*output->w)[3];
      }
    }
  }

  // remap from convolved values to 0-255 range
  for (uint16_t y = ker_l2; y < (input->h)-ker_l2; y++) { // hor (columns) 520
    for (uint16_t x = ker_l2; x < (input->w)-ker_l2; x += 2) { // ver (rows) 240

      (dest + 2*x + 2*y*output->w)[1] = 
            ((dest + 2*x + 2*y*output->w)[1]-min_k1) * 255 / (max_k1-min_k1+1);
      (dest + 2*x + 2*y*output->w)[3] = 
            ((dest + 2*x + 2*y*output->w)[3]-min_k3) * 255 / (max_k3-min_k3+1);

      // only consider as edges those pixels with value > 20 (/new_w) to reduce 
      // noise
      if ((dest + 2*x + 2*y*output->w)[1] > 20) {
        mask_e[y][x] = 1;
        (dest + 2*x + 2*y*output->w)[1] = 255;
      } else {
        mask_e[y][x] = 0;
        (dest + 2*x + 2*y*output->w)[1] = 0;
      }
      if ((dest + 2*x + 2*y*output->w)[3] > 20) {
        mask_e[y][x+1] = 1;   
        (dest + 2*x + 2*y*output->w)[3] = 255; 
      } else {
        mask_e[y][x+1]=0;
        (dest + 2*x + 2*y*output->w)[3] = 0;
      }
    }
  }
}

// Find poles from orange mask
void find_orange_objs(void){

  // Get number of appearances
  for (uint16_t x = 0; x < new_h; x++){
    sums_o[x] = 0;
    for (uint16_t y = 0; y < new_w; y++){
      sums_o[x] += mask_o[x][y];
    }
  }

  // Get derivative in number of appearances
  int16_t der1 = 0;
  count_o = 0;
  bool started = false;

  for (uint16_t x = 1; x < new_h-1; x++){

    // LOGIC WITH THRESHOLD ON 1st DERIVATIVE
    der1 = sums_o[x+1] - sums_o[x-1];

    if (abs(der1) > 50) {
      if (der1 > 0) {
        poles[count_o][0] = x;
        started = true;
      } else {
        poles[count_o][1] = x;
        count_o++;
        started = false;
      }
      x += 8;
    }
  }

  // if an object has detected to begin, but its end has not been found, set end
  // of image as end of obejct
  if (started){
    poles[count_o][1] = new_h-1;
    count_o++;
  }
}

// Find poles from green mask
void find_green_objs(void){

  uint8_t sum;
  uint8_t top_l;

  // Get floor index
  for (int16_t x = 0; x < new_h; x++){
    for (int16_t y =new_w-1; y > -1; y--){
      sum = 0;
      // the first white pixel
      if ((mask_g[x][y] == 1)){
        // define upper boundary seach
        if (y>15){ // if there are more than 8 pixels in the row left
          top_l = 15;
        } else { // else
          top_l = y;
        }
        for (uint8_t i=0; i<top_l; i++){
          sum += mask_g[x][y+i];
        }

        // if not just one random pixel
        if (sum > top_l - 2){
          floors[x] = y;
          break;
        }
      }
      if (y == 0){
        floors[x] = y;
      }
    }
  }

  // check if close to the boundary of the green floor, to avoid identifying
  // floor diagram as object
  check_green = true;

  // if close to the top (x+)
  if (x_loc > top_x_green){
    // and not looking inside the circle
    if ((head < head_x_t_green) && (head > -head_x_t_green)){
      check_green = false;
    }
  } 
  // if close to the bottom (x-)
  if (x_loc < -top_x_green){
    // and not looking inside the circle
    if ((head > head_x_b_green) || (head < -head_x_b_green)){
      check_green = false;
    }
  }
  // if close to the right (y+)
  if (y_loc > top_y_green){
    // and not looking inside the circle
    if ((head > -head_y_t_green) || (head < -head_y_b_green)){
      check_green = false;
    }
  }
  // if close to the left (y-)
  if (y_loc < -top_y_green){
    // and not looking inside the circle
    if ((head < head_y_t_green) || (head > head_y_b_green)){
      check_green = false;
    }
  }

  // Get floor slope from floor indexes
  int8_t der1 = 0;
  count_g = 0;
  bool started = false;

  // Only perform gradient based object search if inside circle and looking 
  // inside to avoid detecting the edge of the floor
  if (check_green){
    for (uint16_t x = 2; x < new_h-2; x++){

      // LOGIC WITH THRESHOLD ON 1st DERIVATIVE
      der1 = floors[x+2] + floors[x+1] - floors[x-1] - floors[x-2];

      if (abs(der1) > 18) {
        if (der1 < 0){
          poles[idx_g+count_g][0] = x;
          started = true;
        } else {
          if ((poles[idx_g+count_g][0] != 0) || (count_g < 0.5)){
            poles[idx_g+count_g][1] = x;
            count_g++;
            started = false;
          }
        }
        x += 8;
      }
    }

    // if an object has detected to begin, but its end has not been found, set
    // end of image as end of obejct
    if (started){
      poles[idx_g+count_g][1] = new_h-1;
      count_g++;
    }
  }
}

// Find poles from edge mask
void find_edge_objs(struct image_t *input){
  // angle from pose to start/end of curtain

  float head_c1 = atan2(( 4.35-y_loc), ( -5.35-x_loc)); // curtain start (-, +) 
  float head_c2 = atan2((-4.35-y_loc), (  5.35-x_loc)); // curtain end (+, -)
  count_e = 0;

  // if looking at: curtain, free, or mix
  if ((head + 0.6981 < head_c1) && (head - 0.6981 > head_c2)){
    // if pointing only at the curtain
    edge_search_type = 1;
  } else {
    // if pointing at crossing 1 (-, +)
    if ((head - 0.6981 < head_c1) && (head + 0.6981 > head_c1)){
      edge_search_type = 2;
      px_change = (head_c1 - head + 0.6981)*0.716197*new_h;
    } else {
      // if pointing at crossing 2 (+, -)
      if ((head - 0.6981 < head_c2) && (head + 0.6981 > head_c2)){
        edge_search_type = 3;
        px_change = (head_c2 - head)*0.716197*new_h*0.94 + new_h/2;
        // 0.94 is the correction factor because mapping is not linear
      } else {
        // if pointing to the curtain-free sides
        edge_search_type = 4;
      }
    }
  }

  if (px_change == 0){
    px_change = 1;
  }

  // Get number of appearances (integral of white pixels per column)
  for (uint16_t x = 0; x < new_h; x++){
    sums_e[x] = 0;
    for (uint16_t y = 0; y < new_w; y++){
      sums_e[x] += mask_e[x][y];
    }
  }

  int8_t min_smooth;
  int16_t max_smooth;
  uint16_t sum;

  // Smooth the sums_e function to get clearer derivatives
  for (uint16_t x = 0; x < new_h; x++){
    if (x < 2){
      min_smooth = -x;
    } else {
      min_smooth = -2;
    }
    if (x > new_h-3){
      max_smooth = new_h-x;
    } else{
      max_smooth = 3;
    }
    sum = 0;
    for (int8_t i = min_smooth; i<max_smooth; i++){
      sum += sums_e[x+i];
    }
    sums_e_smooth[x] = sum/(max_smooth - min_smooth);
  }
  // temp to identify if object has started/finished to be detected
  started_edge = false;

  // curtain all over logic
  if (edge_search_type == 1){
    logic_curtain(1, new_h-1, input);
  }

  // curtain on the left, free on the right logic
  if (edge_search_type == 2){
    logic_curtain(1, px_change-1, input);
    logic_free(px_change, new_h);
  }

  // free on the left, curtain on the right logic
  if (edge_search_type == 3){
    logic_free(1, px_change);
    logic_curtain(px_change+1, new_h, input);
  }

  // free all over logic
  if (edge_search_type == 4){
    logic_free(1, new_h);
  }



  // if an object has detected to begin, but its end has not been found, set end
  // of image as end of obejct
  if (started_edge){
    poles[idx_e+count_e][1] = new_h-1;
    count_e++;
  }

  // Correct measurement if one is behind another
  for (uint16_t x = 0; x<count_e; x++){
    if ((x > 0) && (poles[idx_e+x][0] == 0)){
      poles[idx_e+x][0] = poles[idx_e+x-1][1];
    }

    if ((x < count_e-1) && (poles[idx_e+x][1] == new_h-1)){
      poles[idx_e+x][1] = poles[idx_e+x+1][0];
    }
  }
}

// Find obstacles from edges on curtain side
void logic_curtain(uint16_t idx1, uint16_t idx2, struct image_t *input){  

  int16_t der = 0;
  int8_t lim_sum;
  uint16_t sum_cc_top;
  uint16_t sum_cc_bot;
  uint8_t *source = ((uint8_t *)input->buf);

  // LOGIC WITH THRESHOLD ON 1st DERIVATIVE
  for (uint16_t x = idx1; x < idx2; x++){


    // Get derivative in number of appearances
    der = sums_e_smooth[x+1] - sums_e_smooth[x-1];
    // if the gradient is enough
    if (abs(der) > 25) {

      // identify if start or end object by comparing the colors 
      // (backgroun -> darker)
      if (x < 5){
        lim_sum = -x;
      } else {
        if (x > idx2-5){
          lim_sum = -idx2+x;
        } else{
          lim_sum = -5;
        }
      }

      sum_cc_bot = 0;
      sum_cc_top = 0;

      for (int8_t x_sum = lim_sum; x_sum < 0; x_sum++){
        // color diff at vertical pixel 0.8*img_w (from bottom)
        sum_cc_bot += (source + 2*(new_w-30) + 2*(x+x_sum)*input->w)[1];
        sum_cc_top += (source + 2*(new_w-30) + 2*(x-x_sum)*input->w)[1];
      }

      // If bottom is darker, object starts
      if (sum_cc_bot < sum_cc_top){
        poles[idx_e+count_e][0] = x;
        started_edge = true;
      } else { // else, object is ending

        // check if measurement is not from the previous object on the free side
        if (edge_search_type == 3){
          if (started_edge){
            poles[idx_e+count_e][1] = x;
            started_edge = false;
            count++;
          } else{
            if (count_e > 0){
              if (x-poles[idx_e+count_e-1][1] < 20){
                poles[idx_e+count_e-1][1] = (poles[idx_e+count_e-1][1] + x)/2;
                started_edge = false;
              }
            }
          }
        } else{
          poles[idx_e+count_e][1] = x;
          started_edge = false; 
          count_e++;
        }
      }
      x+=10;
    }
  }  
}

// Find obstacles from edges on curtain-free side
void logic_free(uint16_t idx1, uint16_t idx2){
  uint16_t obj_width;
  uint16_t x_run;

  for (uint16_t x = idx1; x < idx2; x++){

    // Calculate object only if there is a hole (stop) in the edges noise (due
    // to huge amount of noise on curtain-free edge)
    if (sums_e_smooth[x] < 30){
      obj_width = 0;

      for (x_run = x; x_run < idx2; x_run++){
        if (sums_e_smooth[x_run] < 15){
          obj_width++;
        } else {
          break;
        }
      }

      // If no more than 10 pixel edges for 15 pixels width, consider it an
      // object
      if (obj_width > 8){

        // if object already started
        if (x == idx1){
          poles[idx_e+count_e][1] = x_run;
          started_edge = false;
          count_e++;
          x = x_run+5;
        } else { 
          //if object did not finish
          if (x_run == idx2-1){
            poles[idx_e+count_e][0] = x;
            started_edge = true;
          } else{ // else: if object is in the middle (starts and finishes)
            poles[idx_e+count_e][0] = x;
            poles[idx_e+count_e][1] = x_run;
            started_edge = false;
            count_e++;
            x = x_run+5;
          }
        }
      } else{
        if ((edge_search_type == 2) && (x - idx1 < 15)){
          if (started_edge){
            poles[idx_e+count_e][1] = px_change;
            started_edge = false;
          }
        }
      }
    }
  }
}

// Associate measurements from different readings
void combine_measurements(void){
  uint16_t dist;
  count = 0;

  // Merge orange and green objects
  // if no green is detected
  if (count_g == 0){ 
    for (uint8_t co = 0; co < count_o; co++){
      poles_comb[count][0] = poles[co][0];
      poles_comb[count][1] = poles[co][1];
      poles_comb[count][2] = 1;
      count++;
    }
  } else {
    // if no orange is detected
    if (count_o == 0){
      for (uint8_t cg = 0; cg < count_g; cg++){
        poles_comb[count][0] = poles[idx_g+cg][0];
        poles_comb[count][1] = poles[idx_g+cg][1];
        poles_comb[count][2] = 1;
        count++;
      }
    } else {
      for (uint8_t co = 0; co < count_o; co++){
        for (uint8_t cg = 0; cg < count_g; cg++){
          // calculate distance between objects detected
          dist = abs(poles[co][0] - poles[idx_g+cg][0]);

          // when distance between objects is below threshold, objects are 
          // considered as the same and compute averaged values
          if (dist < threshold){
            poles_comb[count][0] = (poles[co][0] + poles[idx_g+cg][0])/2;
            poles_comb[count][1] = (poles[co][1] + poles[idx_g+cg][1])/2;
            poles_comb[count][2] = 1;

            obj_merged[cg] = 1;
            count++;

            break;
          } 

          // if orange pole is not detected by green filter, add it to list
          if (cg == count_g-1) {
            poles_comb[count][0] = poles[co][0];
            poles_comb[count][1] = poles[co][1];
            poles_comb[count][2] = 1;
            count++;
          }
        }
      }

      // add green obj not detected by orange filter
      for (uint8_t cg = 0; cg < count_g; cg++){     
        if (!obj_merged[cg]){
          poles_comb[count][0] = poles[idx_g+cg][0];
          poles_comb[count][1] = poles[idx_g+cg][1];
          poles_comb[count][2] = 0;

          count++;

          // Useful not to compute distance from width if not orange pole
          obj_merged[cg] = 0; 
        }
      }
    }
  }

  // Merge objects from edge detector
  // if no object was detected until now
  if ((count == 0)){
    for (uint8_t cr = 0; cr < count_e; cr++){
      poles_comb[count][0] = poles[idx_e+cr][0];
      poles_comb[count][1] = poles[idx_e+cr][1];
      poles_comb[count][2] = 0;
      count++;
    }
  } else {
    for (uint8_t cr = 0; cr < count_e; cr++){
      for (uint8_t c = 0; c < count; c++){
        // calculate distance between objects detected
        dist = abs(poles[idx_e+cr][0] - poles_comb[c][0]);

        // when distance between objects is below threshold, objects are 
        // considered as the same and compute averaged values
        if (dist < threshold){
          poles_comb[c][0] = (2*poles_comb[c][0] + poles[idx_e+cr][0])/3;
          poles_comb[c][1] = (2*poles_comb[c][1] + poles[idx_e+cr][1])/3;
          break;
        }

        // add objects detected by edge mask not detected by the orange or
        // green mask
        if (c == count-1) {
          poles_comb[count][0] = poles[idx_e+cr][0];
          poles_comb[count][1] = poles[idx_e+cr][1];
          poles_comb[count][2] = 0;
          count++;
        }
      }
    }
  }

  // printf("Combined measurements\n");
  // for (uint16_t x = 0; x < 10; x++){
  //   printf("[%d, %d] \n", poles_comb[x][0], poles_comb[x][1]);
  // }
  // printf("\n");

  // printf("Orange measurements\n");
  // for (uint16_t x = 0; x < 10; x++){
  //   printf("[%d, %d] \n", poles[idx_o+x][0], poles[idx_o+x][1]);
  // }
  // printf("\n");

  // printf("Green measurements\n");
  // for (uint16_t x = 0; x < 10; x++){
  //   printf("[%d, %d] \n", poles[idx_g+x][0], poles[idx_g+x][1]);
  // }
  // printf("\n");


  // printf("Edge measurements\n");
  // for (uint16_t x = 0; x < 10; x++){
  //   printf("[%d, %d] \n", poles[idx_e+x][0], poles[idx_e+x][1]);
  // }
  // printf("\n");


}

// If an object is detected more than XX times in the last XX views
// then take it as valid
void delete_outliers(void){
  uint16_t dist;
  uint8_t new_count = 0;
  uint16_t min_dist;
  uint8_t min_idx;

  // if list obj_w_inertia is empty, add all seen cones
  if (count_inertia == 0){
    for (uint8_t c_old = 0; c_old < count; c_old++){

      // Add object location
      poles_w_inertia[c_old][0] = poles_comb[c_old][0];
      poles_w_inertia[c_old][1] = poles_comb[c_old][1];

      // Add object type
      poles_w_inertia[c_old][2] = poles_comb[c_old][2];

      // Add # of times seen
      poles_w_inertia[c_old][3] = 2;
    }
    new_count = count;
  } else{
    // if object is rotating or moving fast, then take all the measurements
    if ((v_tot > 1) || (rot_loc > 0.4)){
      for (uint8_t c_old = 0; c_old < count; c_old++){

        // Add object location
        poles_w_inertia[c_old][0] = poles_comb[c_old][0];
        poles_w_inertia[c_old][1] = poles_comb[c_old][1];

        // Add object type
        poles_w_inertia[c_old][2] = poles_comb[c_old][2];

        // Add # of times seen
        poles_w_inertia[c_old][3] = 9;

      }
      new_count = count - count_inertia;

    } else{
      // otherwise for all objects seen
      for (uint8_t c_old = 0; c_old < count; c_old++){
        min_dist = 1000;
        for (uint8_t c_inrt = 0; c_inrt < count_inertia; c_inrt++){

          dist = abs(poles_comb[c_old][0] - poles_w_inertia[c_inrt][0]);

          if (dist<min_dist){
            min_dist = dist;
            min_idx = c_inrt;
          }

        if ((c_inrt == count_inertia-1) && (min_dist > threshold)){
            // if object was not seen before
            // Add object location
            poles_w_inertia[count_inertia+new_count][0] = poles_comb[c_old][0];
            poles_w_inertia[count_inertia+new_count][1] = poles_comb[c_old][1];

            // Add object type
            poles_w_inertia[count_inertia+new_count][2] = poles_comb[c_old][2];

            // Add # of times seen
            poles_w_inertia[count_inertia+new_count][3] = 2;

            new_count++;
          }
        }

        // if object was seen before
        if (min_dist < associate_threshold){

          // Update the measurement to new location
          poles_w_inertia[min_idx][0] = poles_comb[c_old][0];
          poles_w_inertia[min_idx][1] = poles_comb[c_old][1];

          // Update type
          poles_w_inertia[min_idx][2] = poles_comb[c_old][2];

          // Update times seen
          poles_w_inertia[min_idx][3] += 2;
        }
      }
    }
  }

  count_inertia += new_count;
  final_count = 0;
  elem_to_rm = 0;


  for (uint8_t c_inrt = 0; c_inrt < count_inertia; c_inrt++){
    poles_w_inertia[c_inrt][3]--;

    // if the obj was seen negative times, add to the list of elems to delete
    if (poles_w_inertia[c_inrt][3] < 1){
      idx_to_rm[elem_to_rm] = c_inrt;
      elem_to_rm ++;

    } else {
      // if the pole has a scole > 7, consider it a valid pole
      if (poles_w_inertia[c_inrt][3] > 7){
        // put capo on the maximum times a cone can be seen
        if (poles_w_inertia[c_inrt][3] > 30){
          poles_w_inertia[c_inrt][3] = 30;
        }
        // Add object location
        final_objs[final_count][0] = poles_w_inertia[c_inrt][0];
        final_objs[final_count][1] = poles_w_inertia[c_inrt][1];

        // Add object type
        final_objs[final_count][3] = poles_w_inertia[c_inrt][2];

        final_count++;
      }
    }
  }

  uint8_t last_idx = count_inertia-1;
  uint8_t rm_idx_i;

  for (uint8_t c_rm = 0; c_rm < elem_to_rm; c_rm++){
    rm_idx_i = idx_to_rm[c_rm];
    for (uint8_t c_mv = rm_idx_i; c_mv < last_idx; c_mv++){
      poles_w_inertia[c_mv][0] = poles_w_inertia[c_mv+1][0];
      poles_w_inertia[c_mv][1] = poles_w_inertia[c_mv+1][1];
      poles_w_inertia[c_mv][2] = poles_w_inertia[c_mv+1][2];
      poles_w_inertia[c_mv][3] = poles_w_inertia[c_mv+1][3];
    }
    poles_w_inertia[last_idx][0] = 0;
    poles_w_inertia[last_idx][1] = 0;
    poles_w_inertia[last_idx][2] = 0;
    poles_w_inertia[last_idx][3] = 0;
    last_idx--;
  }

  count_inertia -= elem_to_rm;

  // printf("Intertial measurements\n");
  // for (uint16_t x = 0; x < 10; x++){
  //   printf("[%d, %d, %d] \n", poles_w_inertia[x][0], poles_w_inertia[x][1], poles_w_inertia[x][3]);
  // }
  // printf("\n");


}

// Estimate distance to object
void find_distances(void){

  int16_t avg_px;
  float head_obj;
  float d_to_edge = 0;

  float dist1;
  float dist2;

  // angle from pose to all four corners of map
  float head_1 = atan2(( 3.85-y_loc), ( 3.85-x_loc)); // top r (+, +) 
  float head_2 = atan2(( 3.85-y_loc), (-3.85-x_loc)); // bot r (+, -)
  float head_3 = atan2((-3.85-y_loc), (-3.85-x_loc)); // bot l (-, -)
  float head_4 = atan2((-3.85-y_loc), ( 3.85-x_loc)); // top l (-, +))

  for (uint8_t idx = 0; idx < final_count; idx++){

    dist1 = 0;
    dist2 = 0;

    // Find distance from size of intrusion on the ground

    avg_px = (final_objs[idx][1]+final_objs[idx][0])/2;
    if (floors[avg_px] != 0){
      // Get angle to obj from linear mapping on the pixel location
      head_obj = head + (-40+4*avg_px*2/new_h) * 0.017453;

      // if pointing to top edge
      if ((head_obj < head_1) && (head_obj > head_4)){
        d_to_edge = (3.85-x_loc) / cos(head_obj);
      }
      // if pointing to the right edge
      if ((head_obj > head_1) && (head_obj < head_2)){
        d_to_edge = (3.85-y_loc) / sin(head_obj);
      }
      // if pointing to the bottom edge
      if ((head_obj > head_2) || (head_obj < head_3)){
        d_to_edge = (-3.85-x_loc) / cos(head_obj);
      }
      // if pointing to the left edge
      if ((head_obj > head_3) && (head_obj < head_4)){
        d_to_edge = (-3.85-y_loc) / sin(head_obj);
      }

      // aritmetic expression that yields approximate results for pole distance
      dist1 = 2*9*d_to_edge/scale_w/(floors[(uint16_t)final_objs[idx][0]] 
                                   + floors[(uint16_t)final_objs[idx][1]]);
      dist1 = sin(dist1);
      dist1 = floors[avg_px]*dist1/9 + 0.9;
    }

    // Find distance from object thickness 
    // (assuming thickness of 0.3 m -> pole thickness)
    dist2 = px_dist_scale/(final_objs[idx][1]-final_objs[idx][0])/2;

    // Only compute distance from width if object was detected by orange filter
    if ((uint8_t)(final_objs[idx][3]) == 1){
      if (dist1 > 0.05){
        final_objs[idx][2] = (2*dist2 + dist1) / 3;
      } else {
        final_objs[idx][2] = dist2;
      }
    } else {
      final_objs[idx][2] = dist1;
    }

    // Sanity check: if distance is not positive, place them at one meter
    if (!(final_objs[idx][2] > 0.01)){
      final_objs[idx][2] = 1;
    }
  }
}

// Map object coordinates to original image size
void remap_to_original_img(void){
  for (uint8_t x = 0; x < final_count; x++){
    final_objs[x][0] = final_objs[x][0]*scale_h;
    final_objs[x][1] = final_objs[x][1]*scale_h;
  }
}

// Initialisation function
void observer_node_init(void){
  cv_add_to_device(&COLORFILTER_CAMERA, observer_func, OPENCVDEMO_FPS);
}