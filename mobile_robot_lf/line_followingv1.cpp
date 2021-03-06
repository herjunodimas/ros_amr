#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <sstream>
#include <geometry_msgs/Twist.h>


  cv::Mat gray, blank;
  cv::Mat edge_detect;


  //Canny threshold info: for small ROI it worked well with lowthresh being 70 and the highthresh being 140, fullview 70low 210high
  int lowthreshold =70; //79 was tested through use of trackbar
  double minx1 =640, maxx2 =0;
  double error = 0;
  double xloc = 320;
  double base = 15;
  double diff = 0;
  double P=0;
  double kp = 0.12;//0.1;
  ros::Publisher pub;


void CallBackFuncMono(const sensor_msgs::ImageConstPtr& frame)
{
  cv_bridge::CvImagePtr cv_ptr;
  try
  {
   cv_ptr  = cv_bridge::toCvCopy(frame, sensor_msgs::image_encodings::MONO8);
  }
  catch (cv_bridge::Exception& e)
  {
    ROS_ERROR("cv_bridge exception: %s", e.what());
  }

 cv::Mat view= cv_ptr->image;
 cv::Mat ROI = cv_ptr->image(cv::Rect(0,430,640,50));
  
////////////////////////////////ROI edge detection//////////////////////
 cv::Mat blank(480,50,CV_8UC1,0);

  cv::blur(ROI,gray,cv::Size(3,3));
  cv::Canny(gray,edge_detect, lowthreshold, lowthreshold*3, 3);

  gray.copyTo(blank, edge_detect);
  
  std::vector<cv::Vec4i> lines;
  cv::HoughLinesP(edge_detect, lines, 1, CV_PI/180,10,10,20);

  minx1 = 640;
  maxx2 = 0;
  for( size_t i = 0; i < lines.size(); i++)
  {
    cv::Vec4i l = lines[i];
    if(l[0] < minx1){
     minx1= l[0];
    }
    if(l[2] > maxx2){
     maxx2 = l[2];
    }
  }
  if(minx1!=640 && maxx2 !=0){
   xloc = (((maxx2-minx1)/2)+minx1);
  }
  error = 320-xloc;

///////////////////////////////ROI edge detection END///////////////////


////////////////////////////////Full image edge detection////////////////      
 /* cv::Mat blank(480,640,CV_8UC1,0);

  cv::blur(cv_ptr->image,gray,cv::Size(3,3));
  cv::Canny(gray,edge_detect, lowthreshold, lowthreshold*3, 3);

  gray.copyTo(blank, edge_detect);

  cv::cvtColor(blank,blank, CV_GRAY2BGR);

  std::vector<cv::Vec4i> lines;
  cv::HoughLinesP(edge_detect, lines, 1, CV_PI/180,50,40,30);
  for( size_t i = 0; i < lines.size(); i++)
  {
    cv::Vec4i l = lines[i];
    cv::line(blank, cv::Point(l[0],l[1]),cv::Point(l[2],l[3]),cv::Scalar(0,0,255), 3, CV_AA);
  }   */
///////////////////////////////Full image edge detection END///////////////

    geometry_msgs::Twist msg;

    P = error*kp;

    diff = P;
    msg.linear.x=base;
    msg.angular.z=diff;
   
    pub.publish(msg);
  
    cv::cvtColor(ROI,ROI, CV_GRAY2BGR);
    cv::circle(ROI, cv::Point((xloc),25),2,cv::Scalar(255,255,255),3);
   // std::ostringstream os;
   // os << "Line Pos: " <<error <<std::endl;
  //  cv::putText(ROI,os.str(),cv::Point(0,45),CV_FONT_HERSHEY_SIMPLEX,0.5,cv::Scalar(255,0,0));
     cv::line(ROI, cv::Point(320,50),cv::Point(320,0),cv::Scalar(0,0,255), 1, CV_AA);
    cv::imshow("Follower" ,ROI);
    cv::imshow("Camera View",view);
    cv::waitKey(10); 

}




int main(int argc, char **argv)
{
  
  ros::init(argc,argv,"line_follower");
  ros::NodeHandle nh;
  cv::namedWindow("Follower");
  cv::namedWindow("Camera View");
  pub=nh.advertise<geometry_msgs::Twist>("/cmd_vel",3);

  image_transport::ImageTransport it(nh);
  image_transport::Subscriber sub = it.subscribe("camera/rgb/image_mono", 1, CallBackFuncMono);

  ros::spin();

  cv::destroyAllWindows();
}
