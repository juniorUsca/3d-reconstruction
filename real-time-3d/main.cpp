#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <math.h>
#include <time.h>
//#include <opencv/cv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

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

double z, z_avance;
vector<ddd> cloud;

void writeData();
void graphData();
double toDegree(double);

double calcularFPS(VideoCapture&);
void processFrame(Mat frame, double angulo);

int main( int argc, char** argv )
{
    VideoCapture cap;
    Mat frame;

    const string direccionFlujoVideo = "http://169.254.162.145:8080/?action=stream";

    if (!cap.open(direccionFlujoVideo)) {
        cout << "Error capturando video" <<endl;
        return -1;
    }

    double FPS = calcularFPS(cap);

    cout << "para iniciar presione CUALQUIER TECLA" << endl;

    while (1) {
        /*if (!cap.read(frame)) {
            cout << "No frame" << endl;
        }*/
        cap >> frame;

        //namedWindow( "Display window", CV_WINDOW_NORMAL);
        imshow("imagen", frame);

        if(cv::waitKey(1) >= 0) break;
    }

    cout << "EMPEZANDO con parametros: " << endl;

    double time_s = 16.; // tiempo en segundos para 180 grados
    double tiempo_mov = 0.7; // tiempo en que se mueve el carro adelante
    double grados = 180;

    double num_frames = FPS * time_s;

    double dist_h = 30.; //distancia al punto de interseccion en centimetros
    double signo = -1.; /// positivo o negativo

    double gradoActual = 180.;
    double gradoAvance = grados / num_frames;

    z = 0.;
    z_avance = 200.;

    cout << "Distancia: " << dist_h << endl;
    cout << "Tiempo para 180 grados: " << time_s << endl;
    cout << "grado x frame: " << gradoAvance << endl;
    cout << "Tiempo moviendose: " << tiempo_mov << endl;


    cout << "para terminar presione cualquier TECLA" << endl;
    //graphData();
    //int tmp = system("gnuplot -p code.gnu");
    time_t start, end;
    while (1) {
        cap >> frame;

        //imshow("imagen", frame);

        cout << gradoActual << endl;
        processFrame(frame, gradoActual);

        gradoActual = gradoActual + (signo*gradoAvance);
        if (gradoActual >= grados) {
            signo = -1.;
            z += z_avance;
            cout << "completo 180" << endl;

            cap.release();
            time(&start);
            while (1) {
                time(&end);
                double seconds = difftime(end, start);
                if(seconds >= tiempo_mov)
                    break;
            }
            cap.open(direccionFlujoVideo);

            cout << "continuando" << endl;
        }
        if (gradoActual <= .0) {
            signo = 1.;
            z += z_avance;
            cout << "completo 0" << endl;

            cap.release();
            time(&start);
            while (1) {
                time(&end);
                double seconds = difftime(end, start);
                if(seconds >= tiempo_mov)
                    break;
            }
            cap.open(direccionFlujoVideo);

            cout << "continuando" << endl;
        }

        writeData();
        cloud.clear();

        if(cv::waitKey(1) >= 0) break;
    }


    cap.release();
}

double calcularFPS(VideoCapture& cap) {
    cout << "Se estiman: " << cap.get(CV_CAP_PROP_FPS) << " FPS" << endl;
    //int num_frames = 120;
    int num_frames = 60;
    time_t start, end;
    Mat frame;
    time(&start);
    for (int i = 0; i < num_frames; ++i) {
        cap >> frame;
    }
    time(&end);

    double seconds = difftime(end, start);

    cout << "Se capturo " << seconds << " segundo, y los FPS reales son: " << num_frames/seconds << endl;
    return num_frames/seconds;
}





void processFrame(Mat frame, double angulo) {

    int width = frame.cols;
    int height = frame.rows;

    if (frame.empty())
        return;

    //Mat img_red = Mat(height, width, CV_8UC3);
    Mat img = Mat(height, width, CV_8UC1);

    for(int y = 0; y < height; ++y)
    {
        const Pixel* Mr = frame.ptr<Pixel>(y);
        for(int x = 0; x < width; ++x) {
            /// 701
            /// 150
            if (Mr[x].z > 200)
                img.at<BinPixel>(y,x) = 255;
            else
                img.at<BinPixel>(y,x) = 0;
        }
    }


    /// alargar

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
    width  = img.cols;
    height = img.rows;



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
                //cloud.push_back( ddd(x, temp_y, z) );

                // ESPEREMOS QUE ESTE EN VERTICAL
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
                //img.at<BinPixel>(y,x) = 0;
            }
            if (ini!=0 && fin!=0) {
                temp_x = ini+((fin - ini)/2);
                //img.at<BinPixel>( y , temp_x ) = 255;

                //imshow("imagen", img);

                //double pre_x = temp_x;
                //double pre_y = 0.0;
                //double pre_z = y + z;

                //double end_x = (pre_x * cos(toDegree(angulo))) - (pre_y * sin(toDegree(angulo)));
                //double end_y = (pre_x * sin(toDegree(angulo))) + (pre_y * cos(toDegree(angulo)));
                //double end_z = pre_z;

                double pre_x = y + z;
                double pre_y = width - temp_x;
                double pre_z = 0.;

                double end_x = pre_x;
                double end_y = (pre_y * cos(toDegree(angulo))) - (pre_z * sin(toDegree(angulo)));
                double end_z = (pre_y * sin(toDegree(angulo))) + (pre_z * cos(toDegree(angulo)));

                cloud.push_back( ddd(end_x, end_y, end_z) );
            }

        }
    }

        //ddd tmp = ddd(2., 2., 0.);
        //ddd tmp_end;
        //tmp_end.x = tmp.x * cos(gradoActual) + tmp.z * sin(gradoActual);
        //tmp_end.y = tmp.y;
        //tmp_end.z = tmp.x * -sin(gradoActual) + tmp.z * cos(gradoActual);

//        tmp_end.x = (tmp.x * cos(toDegree(gradoActual))) - (tmp.y * sin(toDegree(gradoActual)));
//        tmp_end.y = (tmp.x * sin(toDegree(gradoActual))) + (tmp.y * cos(toDegree(gradoActual)));
//        tmp_end.z = tmp.z;

}




void writeData() {
    ofstream file;
    file.open ("cloud.data", std::ofstream::out | std::ofstream::app);

    for (vector<ddd>::iterator it = cloud.begin(); it != cloud.end(); ++it) {
        ddd point = (*it);
        file << point.x << " " << point.y << " " << point.z << endl;
    }
}

void graphData () {
    // replace file
    ofstream file("code.gnu");

    file << "set xlabel 'X'" << endl;
    file << "set ylabel 'Y'" << endl;
    file << "set zlabel 'Z'" << endl;

    file << "set grid" << endl;
    //file << "splot '$cloud' with dots title \"b\" lt rgb \"#0000FF\"" <<endl;
    file << "splot \"cloud.data\" with dots title \"b\" lt rgb \"#0000FF\"" <<endl;

    file.close();
}

double toDegree(double val) {
    return val*PI/180.;
}
