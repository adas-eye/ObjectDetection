/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//     OBJECTS DETECTION AND TRACKING VIA PERCEPTUAL RELEVANCE METRICS     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////



///  FILE INCLUDES  ///
#include "./PRMovement/pr_movement.h"
#include "./PRMovement/bmpreader.h"
#include "./PRMovement/yuv_rgb.h"
#include "./PRMovement/imgutils.h"
#include "./PRMovement/perceptual_relevance_api.h"
#include <math.h>



///  PREPROCESSOR MACROS  ///
#define I_AM_DEBUGGING 0
#define BUFF_SIZE_OBJECT_DETECTION 2
#define MAX_NUM_RECTANGLES 20

#define MIN_PR_DIFF_TO_CONSIDER_CUMULUS 0.25
#define THRESHOLD_KEEP_RECTANGLE_EDGE 0.25

#define TOP_EDGE 1
#define LEFT_EDGE 2
#define RIGHT_EDGE 3
#define BOTTOM_EDGE 4

#define POSSIBLE_CUMULUS 1
#define NOT_A_CUMULUS 0

#define MAX_CUMULUS_SIZE 5

#define MODE_BASE_IMAGE_FIRST_FRAME 1
#define MODE_BASE_IMAGE_INMEDIATE_PREVIOUS_FRAME 2

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))



///  GLOBAL VARIABLES  ///
//int width, height, rgb_channels, linesize;


//  x1, y1
//  +------------------+
//  |                  |
//  |                  |
//  |                  |
//  +------------------+ x2, y2
//
typedef struct {
    int x1;
    int y1;
    int x2;
    int y2;
} Rectangle;

typedef struct {
    int x_center;
    int y_center;
    int cumulus_size;
}Cumulus;

typedef struct{
    int x;
    int y;
} Point;

Rectangle *rectangles;
int rect_index;
int mode;



///  FUNCTION PROTOTYPES  ///
void rectangles_malloc();
void rectangles_free();
int save_rectangle(Rectangle rect);


void pr_changes(int image_position_buffer);


int find_objects();

int is_cumulus_seed(int block_x, int block_y);

Cumulus get_cumulus_centered(int block_x, int block_y);
float* cumulus_pr_neighbours(int block_x, int block_y, int cumulus_size);
float sum_pr_diffs(int x_center, int y_center, int cumulus_size);

Rectangle reduce_rectangle_size(Rectangle rect);
int drop_upper_rows(Rectangle rect);
int drop_lower_rows(Rectangle rect);
int drop_left_columns(Rectangle rect);
int drop_right_columns(Rectangle rect);


void create_frame2();
int drawEdgeOfRectangle(int block_x, int block_y, int whichEdge);
int draw_rectangles_in_frame();



///  FUNCTION IMPLEMENTATIONS  ///

// init_pr_computation taken from perceptual_relevance_api.h (note that BUFF_SIZE is not tunable with the api!!)

// close_pr_computation taken from perceptual_relevance_api.h (note that BUFF_SIZE is not tunable with the api!!) NO ESTA!!

// lhe_advanced_compute_pr_lum  ES INTERNO (por eso es static) y NO HACE FALTA

// lhe_advanced_compute_perceptual_relevance taken from perceptual_relevance_api.h



// SUBSTITUTE with an adapted pr_to_movement to have position buffer tunable or a method to use the differences with the frame 0
void pr_changes(int image_position_buffer) {
    /*int ref_image_position_buffer;
    
    switch (mode) {
        case MODE_BASE_IMAGE_FIRST_FRAME:
            if (image_position_buffer==0) {
                diffs_x = pr_x_buff[0];
                diffs_y = pr_y_buff[0];
                return;
            }
            ref_image_position_buffer = 0;
            break;
        case MODE_BASE_IMAGE_INMEDIATE_PREVIOUS_FRAME:
            ref_image_position_buffer = image_position_buffer - 1;
            if (ref_image_position_buffer < 0)
                ref_image_position_buffer = buff_size - 1;
            break;
        default:
            printf("ERROR: mode not defined\n");
            return;
    }

    
    for (int i = 0; i < total_blocks_width + 1; i++){
        for (int j = 0; j < total_blocks_height + 1; j++){
            diffs_x[j][i] = pr_x_buff[image_position_buffer][j][i] - pr_x_buff[ref_image_position_buffer][j][i];
            diffs_y[j][i] = pr_y_buff[image_position_buffer][j][i] - pr_y_buff[ref_image_position_buffer][j][i];
            if (diffs_x[j][i] < 0) diffs_x[j][i] = -diffs_x[j][i];
            if (diffs_y[j][i] < 0) diffs_y[j][i] = -diffs_y[j][i];
        }
    }*/
    


    /*for (int i = 0; i < total_blocks_width + 1; i++){
        for (int j = 0; j < total_blocks_height + 1; j++){
            if (image_position_buffer==0) {
                diffs_x[j][i] = pr_x_buff[0][j][i];
                diffs_y[j][i] = pr_y_buff[0][j][i];
            } else {
                diffs_x[j][i] = pr_x_buff[image_position_buffer][j][i] - pr_x_buff[0][j][i];
                diffs_y[j][i] = pr_y_buff[image_position_buffer][j][i] - pr_y_buff[0][j][i];
            }
            if (diffs_x[j][i] < 0) diffs_x[j][i] = -diffs_x[j][i];
            if (diffs_y[j][i] < 0) diffs_y[j][i] = -diffs_y[j][i];
        }
    }*/

    int prev_image_position_buffer;
    switch (mode) {
        case MODE_BASE_IMAGE_FIRST_FRAME:
            for (int i = 0; i < total_blocks_width + 1; i++){
                for (int j = 0; j < total_blocks_height + 1; j++){
                    if (image_position_buffer==0) {
                        diffs_x[j][i] = pr_x_buff[0][j][i];
                        diffs_y[j][i] = pr_y_buff[0][j][i];
                    } else {
                        diffs_x[j][i] = pr_x_buff[image_position_buffer][j][i] - pr_x_buff[0][j][i];
                        diffs_y[j][i] = pr_y_buff[image_position_buffer][j][i] - pr_y_buff[0][j][i];
                    }
                    if (diffs_x[j][i] < 0) diffs_x[j][i] = -diffs_x[j][i];
                    if (diffs_y[j][i] < 0) diffs_y[j][i] = -diffs_y[j][i];
                }
            }
            break;


        case MODE_BASE_IMAGE_INMEDIATE_PREVIOUS_FRAME:
            prev_image_position_buffer = image_position_buffer - 1;
            if (prev_image_position_buffer < 0)
                prev_image_position_buffer = buff_size - 1;

            for (int i = 0; i < total_blocks_width + 1; i++){
                for (int j = 0; j < total_blocks_height + 1; j++){
                    diffs_x[j][i] = pr_x_buff[image_position_buffer][j][i] - pr_x_buff[prev_image_position_buffer][j][i];
                    diffs_y[j][i] = pr_y_buff[image_position_buffer][j][i] - pr_y_buff[prev_image_position_buffer][j][i];
                    if (diffs_x[j][i] < 0) diffs_x[j][i] = -diffs_x[j][i];
                    if (diffs_y[j][i] < 0) diffs_y[j][i] = -diffs_y[j][i];
                    //printf("diffs_x: %f, diffs_y %f\n", diffs_x[j][i], diffs_y[j][i]);
                }
            }
            break;
        default:
            printf("ERROR: mode not defined\n");
            return;
    }
}


// get_block_movement taken from perceptual_relevance_api.h NO ESTA!! es interno y no hace falta? creo que es más conveniente otro nombre
// como get_block_accum_pr_lum_diff o get_block_pr_difference o algo así, no necesariamente significa movimiento

// paint_block taken from perceptual_relevance_api.h NO ESTA!!


// CAMBIAR nombre a create_frame ya que el create_frame de pr_movement.c no estará importado, o añadir alguna opcion para que no se muestre la barra
// en  create_frame y use las diferencias con respecto a la anterior o la de referencia guardada
/*void create_frame2() {
    float movement_block = 0;
    float movement = 0;
    int luminance;
    for (int block_y=0; block_y<total_blocks_height; block_y++)      
    {
        for (int block_x=0; block_x<total_blocks_width; block_x++) 
        {
            movement_block = get_block_pr_difference(block_x, block_y);
            luminance = (int)(movement_block*255);
            if (luminance > 255) luminance = 255;
            if (luminance < 0) luminance = 0;
            paint_block(block_x, block_y, luminance);
        }
    }
}*/

/**
 * Allocates resorces for storing the rectangles.
 */
void rectangles_malloc() {
    rectangles = malloc(MAX_NUM_RECTANGLES * sizeof(Rectangle));
}

/**
 * Frees the resorces allocated for storing the rectangles.
 */
void rectangles_free() {
    free(rectangles);
}

/**
 * Computes the sum of the pr differences (with the reference image) of all the blocks in the cumulus. If the given center
 * is out of the borders of the image, a value of 0.0 is returned, in order not to be selected as the best center.
 *
 * @param int x_center
 *      The x coordinate of the block which is the center of the cumulus.    
 * @param int y_center
 *      The y coordinate of the block which is the center of the cumulus.
 * @param int cumulus_size
 *      Indicates the size of the cumulus. 1 means 1x1 cumulus, 2 means 2x2, etc.
 *
 * @return float
 *      The sum of the pr differences (with the reference image) of all the blocks in the cumulus.
 */
float sum_pr_diffs(int x_center, int y_center, int cumulus_size) {
printf("sum_pr_diffs\n");
printf("x_center = %d\n y_center = %d\ncumulus_size = %d\n", x_center, y_center, cumulus_size);
    if ((x_center<0 || y_center<0 || x_center>total_blocks_width || y_center>total_blocks_height)) {
        printf("if ((x_center<0 || y_center<0 || x_center>total_blocks_width || y_center>total_blocks_height))\n");
        return 0.0;
    }

    float sum;
    int x_min, y_min, x_max, y_max;
    int a, b, i;

    i = cumulus_size;
    a = (i-(1+((i-1)%2)))/2;
    b = (i-(i%2))/2;
    printf("a = %d \t b = %d\n", a, b);

    x_min = x_center - a;
    y_min = y_center - a;
    x_max = x_center + b;
    y_max = y_center + b;
    printf("x_min = %d\ty_min = %d\tx_max = %d\ty_max = %d\n", x_min, y_min, x_max, y_max);

    //x_min = x_center - cumulus_size/2;
    //y_min = y_center - cumulus_size/2;
    //x_max = x_center + cumulus_size/2 + cumulus_size%2;
    //y_max = y_center + cumulus_size/2 + cumulus_size%2;

    sum = 0.0;
    for (int x = x_min; x <= x_max; ++x) {   //if ..x<=x_max.. then get_block_movement will segfault (it avegrages with x+1)
        for (int y = y_min; y <= y_max; ++y) {   //same with y
            if (!(x<0 || y<0 || x>total_blocks_width-1 || y>total_blocks_height-1)){       // if NOT out of image bounds
                sum += get_block_movement(x, y);
            }
        }
    }
    return sum;
}

/**
 * Saves a rectangle given in the position 0 of the rectangles array.
 *
 * @param Rectangle rect
 *      The rectangle to save.
 *
 * @return int
 *      Irrelevant/not used.
 */
int save_rectangle(Rectangle rect) {
    (rectangles[0]).x1 = rect.x1;
    (rectangles[0]).y1 = rect.y1;
    (rectangles[0]).x2 = rect.x2;
    (rectangles[0]).y2 = rect.y2;
    printf("Rectangulo guardado:\nx1=%d\ny1=%d\nx2=%d\ny2=%d\n", (rectangles[0]).x1, (rectangles[0]).y1, (rectangles[0]).x2, (rectangles[0]).y2);
}

/**
 * Calculates the pr sum of all the blocks on the cumuli (given block coordinates and cumulus size) and the possible new centers (8 closest neighbours)
 * 
 * @param block_x
 *      The x block coordinate.
 * @param block_y
 *      The y block coordinate.
 * @param cumulus_size
 *      Indicates the size of the cumulus. 1 means 1x1 cumulus, 2 means 2x2, etc.
 *
 * @return float*
 *      Returns the array of sums of PRs. Always 9 positions. Remember to free memory.
 */
float* cumulus_pr_neighbours(int block_x, int block_y, int cumulus_size) {
    int ij_index;
    float* pr_values = malloc(9 * sizeof(float));
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
           ij_index = (j+1) + 3*(i+1);
           pr_values[ij_index] = sum_pr_diffs(block_x+j, block_y+i, cumulus_size);
        }
    }
    return pr_values;
}

/**
 * Checks if a point and its closest environment is relevant enough to be a cumulus.
 *
 * @param int block_x
 *      The x coordinate of the block that is asked to be considered cumulus.    
 * @param int block_y
 *      The y coordinate of the block that is asked to be considered cumulus.
 *
 * @return int
 *      Returns NOT_A_CUMULUS is the coordinates do not meet the requirements to be consedered a cumulus, and POSSIBLE_CUMULUS is they are met.
 */
int is_cumulus_seed(int block_x, int block_y) {
    float *pr_values;
    int is_cumulus = NOT_A_CUMULUS;
    printf("dentro\n");

    if (sum_pr_diffs(block_x, block_y, 1) >= MIN_PR_DIFF_TO_CONSIDER_CUMULUS) {
        printf("if0\n");
        pr_values = cumulus_pr_neighbours(block_x, block_y, 1);

        printf("if1, pr_values computado\n");
        if (!(pr_values[0]==0.0 && pr_values[1]==0.0 && pr_values[2]==0.0 && \
              pr_values[3]==0.0 &&                      pr_values[5]==0.0 && \
              pr_values[6]==0.0 && pr_values[7]==0.0 && pr_values[8]==0.0)) {
            printf("if2\n");
            is_cumulus = POSSIBLE_CUMULUS;
        }
    }
    return is_cumulus;
}

/**
 * Finds the point that best fits as center of a cumulus.
 *
 * @param int block_x
 *      The x coordinate of the starting center.    
 * @param int block_y
 *      The y coordinate of the starting center.
 *
 * @return Point
 *      The point containing the coordinates of the selected center..
 */
Cumulus get_cumulus_centered(int block_x, int block_y) {
    int cumulus_size, index_new_center;
    float *pr_values;
    Cumulus cumulus;
    cumulus.x_center = block_x;
    cumulus.y_center = block_y;

    for (cumulus_size = 3; cumulus_size <= MAX_CUMULUS_SIZE; ++cumulus_size) {
        do{
            index_new_center = 4;   // The current center. If we dont move, we will stop iterating
            pr_values = cumulus_pr_neighbours(cumulus.x_center, cumulus.y_center, cumulus_size);
            for (int i = 0; i < 9; ++i) {
                if (pr_values[index_new_center] < pr_values[i]) {
                    index_new_center = i;
                }
                printf("pr_values[%d] = %f\n", i, pr_values[i]);
            }
            // Update center coordinates
            if (index_new_center != 4) {
                cumulus.x_center += (index_new_center%3)-1;     //x_center += j
                cumulus.y_center += (index_new_center/3)-1;     //y_center += i
                
                printf("index_new_center=%d\n", index_new_center);
                printf("cumulus.x_center=%d\ncumulus.y_center=%d\n", cumulus.x_center, cumulus.y_center);
                printf("in cumulus.cumulus_size = %d\n", cumulus.cumulus_size);
            }
            free(pr_values);
        } while (index_new_center!=4);
        cumulus.cumulus_size = cumulus_size;
    }
    
    printf("out cumulus.cumulus_size = %d\n", cumulus.cumulus_size);

    return cumulus;
}

/**
 * Transforms a cumulus to a rectangle.
 *
 * @param Cumulus cumulus
 *      The cumulus to transform.    
 *
 * @return Rectangle
 *      The rectangle that encloses the cumulus.
 */
Rectangle cumulus_to_rectangle(Cumulus cumulus){
    printf("cumulus to rectangle\n");
    printf("x_center = %d\n y_center = %d\ncumulus_size = %d\n", cumulus.x_center, cumulus.y_center, cumulus.cumulus_size);
    Rectangle rect;
    int a, b, i;

    i = cumulus.cumulus_size;
    a = (i-(1+((i-1)%2)))/2;
    b = (i-(i%2))/2;
    printf("a = %d \t b = %d\n", a, b);

    rect.x1 = cumulus.x_center - a;
    rect.y1 = cumulus.y_center - a;
    rect.x2 = cumulus.x_center + b;
    rect.y2 = cumulus.y_center + b;
    printf("rect.x1 = %d\trect.y1 = %d\trect.x2 = %d\trect.y2 = %d\n", rect.x1, rect.y1, rect.x2, rect.y2);

    //rect.x1 = cumulus.x_center - (cumulus.cumulus_size-(cumulus.cumulus_size%2)/2);
    //rect.y1 = cumulus.y_center - (cumulus.cumulus_size-(cumulus.cumulus_size%2)/2);
    //rect.x2 = cumulus.x_center + (cumulus.cumulus_size-(cumulus.cumulus_size%2)/2) + cumulus.cumulus_size%2;
    //rect.y2 = cumulus.y_center + (cumulus.cumulus_size-(cumulus.cumulus_size%2)/2) + cumulus.cumulus_size%2;

    //rect.x1 = cumulus.x_center - (cumulus.cumulus_size-(cumulus.cumulus_size%2)/2) - cumulus.cumulus_size%2;
    //rect.y1 = cumulus.y_center - (cumulus.cumulus_size-(cumulus.cumulus_size%2)/2) - cumulus.cumulus_size%2;
    //rect.x2 = cumulus.x_center + (cumulus.cumulus_size-(cumulus.cumulus_size%2)/2);
    //rect.y2 = cumulus.y_center + (cumulus.cumulus_size-(cumulus.cumulus_size%2)/2);

    //rect.x1 = cumulus.x_center - cumulus.cumulus_size/2;                             //X1_FROM_CUMULUS(cumulus);
    //rect.y1 = cumulus.y_center - cumulus.cumulus_size/2;                             //Y1_FROM_CUMULUS(cumulus);
    //rect.x2 = cumulus.x_center + cumulus.cumulus_size/2 + (cumulus).cumulus_size%2;  //X2_FROM_CUMULUS(cumulus);
    //rect.y2 = cumulus.y_center + cumulus.cumulus_size/2 + (cumulus).cumulus_size%2;  //Y2_FROM_CUMULUS(cumulus);

    return rect;
}

/**
 * These four functions keep the same structure. They find the number of border lines that can be eliminated from one side (top/bottom/left/right)
 * of the rectangle to keep it the smallest but still emcompassing the object.
 *
 * @param Rectangle rect
 *      The rectangle to work with.    
 *
 * @return int
 *      The number of lines to remove (0 to keep it the same).
 */
int drop_upper_rows(Rectangle rect) {
    float max_pr_in_the_line;
    int deleted_rows = 0;
    int x1 = rect.x1;
    int y1 = rect.y1;
    int x2 = rect.x2;
    int y2 = rect.y2; 

    for (int i = y1; i <= y2; ++i) {
        max_pr_in_the_line = 0.0;
        for (int j = x1; j <=x2; ++j) {
            max_pr_in_the_line = MAX(max_pr_in_the_line, sum_pr_diffs(j, i, 1));
        }
        if (max_pr_in_the_line < THRESHOLD_KEEP_RECTANGLE_EDGE) {
            deleted_rows++;
        } else {
            return deleted_rows;
        }
    }
    return deleted_rows;
}
int drop_lower_rows(Rectangle rect) {
    float max_pr_in_the_line;
    int deleted_rows = 0;
    int x1 = rect.x1;
    int y1 = rect.y1;
    int x2 = rect.x2;
    int y2 = rect.y2;

    for (int i = y2; i >= y1; --i) {
        max_pr_in_the_line = 0.0;
        for (int j = x1; j <= x2; ++j) {
            max_pr_in_the_line = MAX(max_pr_in_the_line, sum_pr_diffs(j, i, 1));
        }
        if (max_pr_in_the_line < THRESHOLD_KEEP_RECTANGLE_EDGE) {
            deleted_rows++;
        } else {
            return deleted_rows;
        }
    }
    return deleted_rows;
}
int drop_left_columns(Rectangle rect) {
    float max_pr_in_the_line;
    int deleted_cols = 0;
    int x1 = rect.x1;
    int y1 = rect.y1;
    int x2 = rect.x2;
    int y2 = rect.y2;

    for (int j = x1; j <= x2; ++j) {
        max_pr_in_the_line = 0.0;
        for (int i = y1; i <= y2; ++i) {
            max_pr_in_the_line = MAX(max_pr_in_the_line, sum_pr_diffs(j, i, 1));
        }
        if (max_pr_in_the_line < THRESHOLD_KEEP_RECTANGLE_EDGE) {
            deleted_cols++;
        } else {
            return deleted_cols;
        }
    }
    return deleted_cols;
}
int drop_right_columns(Rectangle rect) {
    float max_pr_in_the_line;
    int deleted_cols = 0;
    int x1 = rect.x1;
    int y1 = rect.y1;
    int x2 = rect.x2;
    int y2 = rect.y2;

    for (int j = x2; j >= x1; --j) {
        max_pr_in_the_line = 0.0;
        for (int i = y1; i <= y2; ++i) {
            max_pr_in_the_line = MAX(max_pr_in_the_line, sum_pr_diffs(j, i, 1));
        }
        if (max_pr_in_the_line < THRESHOLD_KEEP_RECTANGLE_EDGE) {
            deleted_cols++;
        } else {
            break;
        }
    }
    return deleted_cols;
}

/**
 * Given a rectangle, keeps or reduces its size to convert it in the smallest rectangle possible that emcompasses the object.
 *
 * @param Rectnagle rect
 *      The rectangle to work with.    
 *
 * @return Rectangle
 *      The reduced size rectangle.
 */
Rectangle reduce_rectangle_size(Rectangle rect) {
    rect.y1 += drop_upper_rows(rect);
    rect.y2 -= drop_lower_rows(rect);
    rect.x1 += drop_left_columns(rect);
    rect.x2 -= drop_right_columns(rect);
    
    return rect;
}

/**
 * Finds the objects in the frame. Starts analyzing the possible cumuli of blocks with high pr difference relative to the base image. After an
 *      iterative process of finding the best cumuli center and refining the size, defines the rectangle that encloses the object and saves it.
 *
 * @return int
 *      Irrelevant/not used.
 */
int find_objects() {
    Rectangle rect;
    //Point point_center;
    Cumulus cumulus;
    printf("finding...\n");
    for (int block_y=0; block_y<total_blocks_height; block_y++) {
        for (int block_x=0; block_x<total_blocks_width; block_x++) {
            printf("block_x=%d - - - block_y=%d\n", block_x, block_y);

            if (is_cumulus_seed(block_x, block_y)){
                printf("cumulus seed found. block_x = %d \tblock_y = %d\n", block_x, block_y);
                cumulus = get_cumulus_centered(block_x, block_y);
                rect = cumulus_to_rectangle(cumulus);
                //rect.y1 = point_center.y - 2;
                //rect.y2 = point_center.y + 2;
                //rect.x1 = point_center.x - 2;
                //rect.x2 = point_center.x + 2;
                rect = reduce_rectangle_size(rect);

                save_rectangle(rect);
                return 0; //just need one rectangle for this version
            }
        }
    }
}

/**
 * Draws the specified edge in the block specified changing the luminance and chrominances in the corresponding edge of the block.
 *
 * @param int block_x
 *      The x coordinate of the block in which the rectangle edge should be drawn.    
 * @param int block_y
 *      The y coordinate of the block in which the rectangle edge should be drawn.
 * @param int whichEdge
 *      Indicates which of the edges should be drawn in the block (top, bottom, left or right). Implemented with preprocesor definitions.
 *
 * @return int
 *      If everything is fine returns 0, if there was a problem, -1.
 */
int drawEdgeOfRectangle(int block_x, int block_y, int whichEdge) {
    int xini = block_x*theoretical_block_width;
    int xfin = xini+theoretical_block_width;
    int yini = block_y*theoretical_block_height;
    int yfin = yini+theoretical_block_height;

    if (xfin > width-1-theoretical_block_width)
        xfin = width;
    if (yfin > height-1-theoretical_block_height)
        yfin = height;

    switch (whichEdge) {
        case TOP_EDGE:
            for (int xx = xini; xx < xfin; xx++) {
                y[yini*width+xx] = 82;
                y[(yini+1)*width+xx] = 82;
            }
            for (int xx = xini/2; xx < xfin/2; xx++) {
                u[yini/2*width+xx] = 90;
                v[yini/2*width+xx] = 240;
            }
        break;

        case LEFT_EDGE:
            for (int yy = yini; yy < yfin; yy++) {
                y[yy*width+xini] = 82;
                y[yy*width+(xini+1)] = 82;
            }
            for (int yy = yini/2; yy < yfin/2; yy++) {
                u[yy*width+xini/2] = 90;
                v[yy*width+xini/2] = 240;
            }
        break;

        case RIGHT_EDGE:
        for (int yy = yini; yy < yfin; yy++) {
                y[yy*width+(xfin-1)] = 82;
                y[yy*width+(xfin-2)] = 82;
            }
            for (int yy = yini/2; yy < yfin/2; yy++) {
                u[yy*width+(xfin/2-1)] = 90;
                v[yy*width+(xfin/2-1)] = 240;
            }       
        break;

        case BOTTOM_EDGE:
            for (int xx = xini; xx < xfin; xx++) {
                y[(yfin-1)*width+xx] = 82;
                y[(yfin-2)*width+xx] = 82;
            }
            for (int xx = xini/2; xx < xfin/2; xx++) {
                u[(yfin/2-1)*width+xx] = 90;
                v[(yfin/2-1)*width+xx] = 240;
            }
        break;

        default:
            printf("ERROR\n");
            return -1;
    }
    return 0;
}

/**
 * Draws the rectangles in the frame changing the luminance and chrominances in the edges of the rectangles.
 *
 * @return int
 *      Irrelevant/not used.
 */
int draw_rectangles_in_frame() {
    Rectangle rect;
    // for each rectangle
    for (int i = 0; i <= rect_index; ++i) {
        rect = rectangles[i];
        for (int block_y = rect.y1; block_y <= rect.y2; block_y++) {
            for (int block_x = rect.x1; block_x <= rect.x2; block_x++) {
                // Draw top edges
                if (block_y==rect.y1){
                    drawEdgeOfRectangle(block_x, block_y, TOP_EDGE);
                }
                // Draw left edge
                if (block_x==rect.x1){
                    drawEdgeOfRectangle(block_x, block_y, LEFT_EDGE);
                }
                // Draw right edge
                if (block_x==rect.x2){
                    drawEdgeOfRectangle(block_x, block_y, RIGHT_EDGE);
                }
                // Draw bottom edge
                if (block_y==rect.y2){
                    drawEdgeOfRectangle(block_x, block_y, BOTTOM_EDGE);
                }
            }
        }
    }
}



///  MAIN  ///
int main( int argc, char** argv ) {
    mode = MODE_BASE_IMAGE_INMEDIATE_PREVIOUS_FRAME; //MODE_BASE_IMAGE_INMEDIATE_PREVIOUS_FRAME    MODE_BASE_IMAGE_FIRST_FRAME
    

    char* imageName = argv[1];
    char* imageExt = argv[2];
    char* frameArg = argv[3];
    int frameNumber;
    char image[100];
    int frame = atoi(frameArg);
    initiated = 0;

    int starting_frame;

    if (mode==MODE_BASE_IMAGE_FIRST_FRAME){
        starting_frame = 0;
        buff_size = 2;
    }
    else if (mode == MODE_BASE_IMAGE_INMEDIATE_PREVIOUS_FRAME){
        starting_frame = 1;
        buff_size = BUFF_SIZE_OBJECT_DETECTION;
    }

    // Loop to process all the frames required
    for (frameNumber = starting_frame; frameNumber < frame; frameNumber++){

        if (frameArg == NULL){
            frameNumber = 0;
            strcpy(image, imageName);
            strcat(image, imageExt);
        } else {
            sprintf(frameArg,"%i",frameNumber);
            strcpy(image, imageName);
            strcat(image, frameArg);
            strcat(image, imageExt);
        }

        printf("%s\n", image);

        BITMAPINFOHEADER bitmapInfoHeader;

        LoadBitmapFileProperties(image, &bitmapInfoHeader);
        width = bitmapInfoHeader.biWidth;
        height = bitmapInfoHeader.biHeight;
        rgb_channels = bitmapInfoHeader.biBitCount/8;

        if (initiated == 0) {
            init_pr_computation(width, height, rgb_channels);
            rectangles_malloc();
            initiated = 1;
        }

        rgb = load_frame(image, width, height, rgb_channels);

        const size_t y_stride = width + (16-width%16)%16;
        const size_t uv_stride = y_stride;
        const size_t rgb_stride = width*3 +(16-(3*width)%16)%16;
        
        rgb24_yuv420_std(width, height, rgb, rgb_stride, y, u, v, y_stride, uv_stride, YCBCR_601);
        
        int position; //int position = MIN(frameNumber, 1); //int position = (frameNumber-1)%BUFF_SIZE_OBJECT_DETECTION;
        if (mode==MODE_BASE_IMAGE_FIRST_FRAME)
            position = MIN(frameNumber, 1);
        else if (mode==MODE_BASE_IMAGE_INMEDIATE_PREVIOUS_FRAME)
            position = (frameNumber-1)%BUFF_SIZE_OBJECT_DETECTION;

        lhe_advanced_compute_perceptual_relevance (y, pr_x_buff[position], pr_y_buff[position]);

        pr_changes(position);// cambiar por diferencia de pr con la primera imagen
        //float movement = get_image_movement(0);
        //printf("MOVEMENT: %f frame: %d\n", movement, frameNumber);

        create_frame(0);

        if (frameNumber > 1 || (frameNumber==1 && mode==MODE_BASE_IMAGE_FIRST_FRAME)) {
            find_objects();
            draw_rectangles_in_frame();
        }

        char frameName[100];
        sprintf(frameName,"./output/output%i.bmp",frameNumber);
        //printf("Frame name: %s\n", frameName);
        yuv420_rgb24_std(width, height, y, u, v, y_stride, uv_stride, rec_rgb, rgb_stride, YCBCR_601);
        stbi_write_bmp(frameName, width, height, rgb_channels, rec_rgb);

    }
    printf("fin1\n");
    close_pr_computation();
    printf("fin2\n");
    rectangles_free();

    return 0;
}


