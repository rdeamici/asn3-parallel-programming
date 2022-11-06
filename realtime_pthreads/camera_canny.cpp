/**
*/
#include <ctime>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include "opencv2/opencv.hpp"
#include "canny_util.h"

using namespace std;
using namespace cv;

/* Possible options: 320x240, 640x480, 1024x768, 1280x1040, and so on. */
/* Pi Camera MAX resolution: 2592x1944 */

#define WIDTH 640
#define HEIGHT 480
#define NFRAME 1.0


int main(int argc, char **argv)
{
   using namespace std::chrono;

   auto wall_start = steady_clock::now();

   char* dirfilename;        /* Name of the output gradient direction image */
   char outfilename[128];    /* Name of the output "edge" image */
   char composedfname[128];  /* Name of the output "direction" image */
   unsigned char *image;     /* The input image */
   unsigned char *edge;      /* The output edge image */
   int rows, cols, numimages, cur_image=1;           /* The dimensions of the image. */
   float sigma,              /* Standard deviation of the gaussian kernel. */
	 tlow,               /* Fraction of the high threshold in hysteresis. */
	 thigh;              /* High hysteresis threshold control. The actual
			        threshold is the (100 * thigh) percentage point
			        in the histogram of the magnitude of the
			        gradient image that passes non-maximal
			        suppression. */

   /****************************************************************************
   * Get the command line arguments.
   ****************************************************************************/
   if(argc < 5){
   fprintf(stderr,"\n<USAGE> %s sigma tlow thigh numimages [writedirim]\n",argv[0]);
      fprintf(stderr,"      sigma:      Standard deviation of the gaussian");
      fprintf(stderr," blur kernel.\n");
      fprintf(stderr,"      tlow:       Fraction (0.0-1.0) of the high ");
      fprintf(stderr,"edge strength threshold.\n");
      fprintf(stderr,"      thigh:      Fraction (0.0-1.0) of the distribution");
      fprintf(stderr," of non-zero edge\n                  strengths for ");
      fprintf(stderr,"hysteresis. The fraction is used to compute\n");
      fprintf(stderr,"                  the high edge strength threshold.\n");
      fprintf(stderr,"      numimages:  argument to control number ");
      fprintf(stderr,                  "of images to process.\n");
      fprintf(stderr,"      writedirim: Optional argument to output ");
      fprintf(stderr,                  "a floating point ");
      fprintf(stderr,                  "direction image.\n\n");
      exit(1);
   }

   sigma = atof(argv[1]);
   tlow = atof(argv[2]);
   thigh = atof(argv[3]);
   numimages = atoi(argv[4]);

   rows = HEIGHT;
   cols = WIDTH;

   if(argc == 6) dirfilename = (char *) "dummy";
	 else dirfilename = NULL;

   auto begin_camera_connection = steady_clock::now();
   VideoCapture cap;
   // open the default camera (/dev/video0)
   // Check VideoCapture documentation for more details
   if(!cap.open(0)) {
      cout<<"Failed to open /dev/video0"<<endl;
      return 0;
   }
   cap.set(CAP_PROP_FRAME_WIDTH, WIDTH);
   cap.set(CAP_PROP_FRAME_HEIGHT,HEIGHT);

   auto end_camera_connection = steady_clock::now();
   Mat frame, grayframe;
   printf("[INFO] taking %d images in a row...\n", numimages);
   printf("----------------------\n");

   clock_t begin, mid, end;
   double time_elapsed, time_capture, time_process, total_time_elapsed = 0;

   auto begin_frame_capture = steady_clock::now();
   while (cur_image <= numimages)
   {
      begin = clock();
      //capture
      cap >> frame;
      mid = clock();
      cvtColor(frame, grayframe, COLOR_BGR2GRAY);
      image = grayframe.data;

      /****************************************************************************
      * Perform the edge detection. All of the work takes place here.
      ****************************************************************************/
      if(VERBOSE) printf("Starting Canny edge detection.\n");

      if(dirfilename != NULL){
         sprintf(composedfname, "camera_s_%3.2f_l_%3.2f_h_%3.2f_frame%03d.fim",
         sigma, tlow, thigh, cur_image);
         dirfilename = composedfname;
      }
      canny(image, rows, cols, sigma, tlow, thigh, &edge, dirfilename);

      /****************************************************************************
      * Write out the edge image to a file.
      ****************************************************************************/
      sprintf(outfilename, "frame%03d.pgm", cur_image);
      if(VERBOSE) printf("Writing the edge iname in the file %s.\n", outfilename);
      if(write_pgm_image(outfilename, edge, rows, cols, NULL, 255) == 0){
         fprintf(stderr, "Error writing the edge image, %s.\n", outfilename);
         exit(1);
      }
      end = clock();
      time_elapsed = (double) (end - begin) / CLOCKS_PER_SEC;
      time_capture = (double) (mid - begin) / CLOCKS_PER_SEC;
      time_process = (double) (end - mid) / CLOCKS_PER_SEC;
      total_time_elapsed += time_elapsed;

      printf("Elapsed time for capturing+processing frame %d: %lf + %lf => %lf seconds\n", cur_image, time_capture, time_process, time_elapsed);
      printf("current FPS: %01lf\n",NFRAME/time_elapsed);

      cur_image++;
   }

   auto wall_end = steady_clock::now();
   // chrono::duration<double> elapsed_seconds = wall_end - wall_start;
   auto wall_ms = duration_cast<milliseconds>(wall_end-wall_start);
   auto camera_connection_time = duration_cast<milliseconds>(end_camera_connection - begin_camera_connection);
   auto frame_capture_time = duration_cast<milliseconds>(wall_end - begin_frame_capture);
   printf("----------------------\n");
   printf("FINISHED\nTOTAL CPU TIME: %01lf\n",total_time_elapsed);
   printf("AVERAGE FPS: %01lf\n", (double) numimages/total_time_elapsed);

   cout << "\nTime taken to establish connection to camera = " << camera_connection_time.count() << " milliseconds" << endl;
   cout << "Time taken to capture and process all " << numimages << " frames = " << frame_capture_time.count() << " milliseconds" << endl;
   cout << "Total time for program to run program = " << wall_ms.count() << " milliseconds" << endl;

   return 0;
}

