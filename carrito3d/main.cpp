#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <math.h>
#include <opencv/cv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <dirent.h>

#define PI 3.14159265


typedef cv::Point3_<uint8_t> Pixel;
typedef uchar BinPixel;

struct iii{
    int x;
    int y;
    int z;
    iii() {}
    iii(int x, int y, int z): x(x),y(y), z(z) {}
};

struct ddd{
    double x;
    double y;
    double z;
    ddd() {}
    ddd(double x, double y, double z): x(x),y(y), z(z) {}
};

using namespace std;
using namespace cv;

static string folderName;
vector<string> frameNames;

int width;
int height;
int num_frames;
#define VERTICAL 0
#define HORIZONTAL 1
int line_orientation = VERTICAL;// 0 vertical, 1 horizontal

vector<ddd>  cloud;

bool orderImages(string);
void processFrames();
void graphData();
double toDegree(double);


int main( int argc, char** argv )
{
    if (argc < 2) {
        printf("!!! Carpeta por defecto \"data\" \n");
        folderName = "data";
    }
    else
        folderName = argv[1];

    if(folderName[folderName.size()-1] == '/') folderName.pop_back();

    if ( !orderImages(folderName) ) return 1;
    /// processing every frame
    processFrames();

    graphData();

    return 0;
}



bool orderImages(string folderName) {
    DIR *p_dir;
    struct dirent *pDirent;

    vector<int> frameNumber;
    string frameName;

    p_dir = opendir(folderName.c_str());
    if (p_dir == NULL) {
        printf ("Cannot open directory '%s'\n", folderName.c_str());
        return false;
    }

    frameNumber.clear();
    while ((pDirent = readdir(p_dir)) != NULL) {
        frameName = pDirent->d_name;
        if(frameName==".") continue;
        if(frameName=="..") continue;
        int foundStart=frameName.find('_');
        int foundEnd=frameName.find('.');

        //cout << foundStart << " " << foundEnd << endl;

        frameName = frameName.substr(foundStart +1, foundEnd-foundStart-1);

        frameNumber.push_back( atoi(frameName.c_str()) );

    }
    closedir(p_dir);

    sort( frameNumber.begin(), frameNumber.end() );

    num_frames = frameNumber.size();

    for ( vector<int>::iterator it = frameNumber.begin(); it!=frameNumber.end(); ++it )
        frameNames.push_back( "frame_" + to_string(*it) + ".jpg" );
    return true;
}

void processFrames() {
    double time = 5.; // tiempo en segundos para 360 grados
    double dist_h = 30.; //distancia al punto de interseccion en centimetros
    int num_frames = frameNames.size();
    double grados = 360;
    double signo = 1.; /// positivo o negativo

    double gradoActual = .0;
    double gradoAvance = signo * (grados / (double)num_frames);


    cout << cos(0.) << endl;
    cout << sin(0.) << endl;

    int z = 0;
    for (vector<string>::iterator i = frameNames.begin(); i != frameNames.end(); ++i,++z) {

        //string frame_name =
        cout << "processing frame: " << *i << endl;




        Mat I = imread( folderName+"/"+(*i) , IMREAD_COLOR);

        //cout << I.cols << endl;
        //cout << I.rows << endl;
        width = I.cols;
        height = I.rows;

        //cv::Rect myROI(860, 0, 960, 800);

        //I = I(myROI);

        //Mat I = imread(frameName.c_str(), IMREAD_GRAYSCALE);
        if (I.empty())
        {
            std::cout << "!!! Failed imread(): image not found" << std::endl;
            continue;
        }

        Mat img_red = Mat(height, width, CV_8UC3);
        Mat img = Mat(height, width, CV_8UC1);

        for(int y = 0; y < height; ++y)
        {
            const Pixel* Mr = I.ptr<Pixel>(y);
            for(int x = 0; x < width; ++x) {
                /// 70
                /// 150
                if (Mr[x].z > 200)
                    img.at<BinPixel>(y,x) = 255;
                //cout << (int)I.at<BinPixel>(i,j) << endl;
                //if (I.at<BinPixel>(i,j) > 40) img_bw.at<BinPixel>(i,j) = 255;
                else
                    img.at<BinPixel>(y,x) = 0;
            }
        }


        /// alargar
        //Rect myROI(80, 80, 350, 239);
        //Mat img = img( myROI );

        //IplImage* img_dst = cvCreateImage(CvSize(), img.depth(), img.channels());
        int fac = 4;
        int fac_x, fac_y;
        if (line_orientation==HORIZONTAL) {
            fac_x = 1;
            fac_y = 4;
        } else {
            fac_x = 4;
            fac_y = 1;
        }

        Mat img_dst(img.rows*fac_y, img.cols*fac_x, CV_8UC1);
        resize(img,img_dst,img_dst.size());

        img.release();
        img = img_dst;
        img_dst.release();
        height = img.rows;
        width  = img.cols;


        /// ESQUELETIZACION: hacer mas delgada la linea

        int ini, fin, val;
        bool changed;
        int temp_x, temp_y;
        if (line_orientation==HORIZONTAL) {
            for(int x = 0; x < width; ++x) {
                ini = 0;
                fin = 0;
                for(int y = 0; y < height; ++y) {
                    val = (int)img.at<BinPixel>(y,x);

                    if (val == 255 && ini == 0)
                        ini = y;
                    if (val == 0 && ini != 0)
                        fin = y;
                    img.at<BinPixel>(y,x) = 0;
                }
                if (ini!=0 && fin!=0) {
                    temp_y = ini+((fin - ini)/2);
                    img.at<BinPixel>( temp_y , x ) = 255;
                    cloud.push_back( ddd(x, temp_y, z) );
                }

            }
        } else {
            for(int y = 0; y < height; ++y) {
                ini = 0;
                fin = 0;
                changed = false;
                for(int x = 0; x < width; ++x) {
                    val = (int)img.at<BinPixel>(y,x);

                    if (val == 255 && ini == 0)
                        ini = x;
                    if (val == 0 && ini != 0 && !changed) {
                        fin = x;
                        changed = true;
                    }
                    img.at<BinPixel>(y,x) = 0;
                }
                if (ini!=0 && fin!=0) {
                    temp_x = ini+((fin - ini)/2);
                    img.at<BinPixel>( y , temp_x ) = 255;


                    double pre_x = temp_x;
                    double pre_y = 0.0;
                    double pre_z = y;
                    double end_x = (pre_x * cos(toDegree(gradoActual))) - (pre_y * sin(toDegree(gradoActual)));
                    double end_y = (pre_x * sin(toDegree(gradoActual))) + (pre_y * cos(toDegree(gradoActual)));
                    double end_z = pre_z;


                    cloud.push_back( ddd(end_x, end_y, end_z) );
                }

            }
        }

        //ddd tmp = ddd(2., 2., 0.);
        //ddd tmp_end;
        //tmp_end.x = tmp.x * cos(gradoActual) + tmp.z * sin(gradoActual);
        //tmp_end.y = tmp.y;
        //tmp_end.z = tmp.x * -sin(gradoActual) + tmp.z * cos(gradoActual);

        /*tmp_end.x = (tmp.x * cos(toDegree(gradoActual))) - (tmp.y * sin(toDegree(gradoActual)));
        tmp_end.y = (tmp.x * sin(toDegree(gradoActual))) + (tmp.y * cos(toDegree(gradoActual)));
        tmp_end.z = tmp.z;


        cloud.push_back( tmp_end );*/


        /*namedWindow( "Display window", CV_WINDOW_NORMAL);// Create a window for display.
        imshow( "Display window", img);
        waitKey(0);*/

        gradoActual += gradoAvance;



    }
}


void graphData () {
    // replace file
    ofstream file("code.gnu");

    file << "set xlabel 'X'" << endl;
    file << "set ylabel 'Y'" << endl;
    file << "set zlabel 'Z'" << endl;

    file << "set grid" << endl;
    //file << "set hidden3d" << endl;
    file << "set dgrid3d 50,50 qnorm 2" << endl;
    //file << "set dgrid3d 100,100,4" << endl;
    file << "set pm3d explicit" << endl;

    file << "$cloud << EOD" << endl;

    for (vector<ddd>::iterator it = cloud.begin(); it != cloud.end(); ++it) {
        ddd point = (*it);
        file << point.x << " " << point.y << " " << point.z << endl;
    }

    file << "EOD" << endl;

    file << "set table \"interpolated_data.dat\"" << endl;
    file << "splot '$cloud'" << endl;
    file << "unset table" << endl;
    file << "unset dgrid3d" << endl;

    //file << "splot \"interpolated_data.dat\" with pm3d title \"a\", " ;
    file << "splot '$cloud' with dots title \"b\" lt rgb \"#0000FF\"" <<endl;

    //file << "splot '$cloud'" <<endl;
    file.close();

    int tmp = system("gnuplot -p code.gnu");
}

double toDegree(double val) {
    return val*PI/180.;
}
