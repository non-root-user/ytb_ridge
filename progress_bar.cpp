#ifdef __APPLE_CC__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <iostream>
#include <chrono>
#include <regex>
#include <curl/curl.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

struct songInfo{
  int id;
  int start_timestamp;
  int length;
};

static const unsigned int width = 1000;
static const unsigned int height = 20;
static const unsigned int FPS = 50;
static int totalTime = 10;

static int color1[3] = {217, 0, 11};
static int color2[3] = {148, 0, 217};
static float changeTime = 3.0;
static int frameLength = changeTime * FPS;
static float incValues[3] = {
  (((float)color1[0] / 255) - ((float)color2[0] / 255)) / (float)frameLength,
  (((float)color1[1] / 255) - ((float)color2[1] / 255)) / (float)frameLength,
  (((float)color1[2] / 255) - ((float)color2[2] / 255)) / (float)frameLength
  };

static int currentSongId = 0;
static int currentFrame = 0;
static GLfloat currentPos = 0.0;

static float currentColor[3] = {(float)color1[0] / 255, (float)color1[1] / 255, (float)color1[2] / 255};

void newColor(){

  if(!(currentFrame % (4 * frameLength))){
    for(int i = 0; i < 3; i++){
      currentColor[i] = (float)color2[i] / 255;
      incValues[i] = incValues[i] * -1;
    }
  }
  else if(!(currentFrame % (2 * frameLength))){
    for(int i = 0; i < 3; i++){
      currentColor[i] = (float)color1[i] / 255;
      incValues[i] = incValues[i] * -1;
    }
  }
  else {
    for(int i = 0; i < 3; i++)
      currentColor[i] = currentColor[i] + (((float)(currentFrame % frameLength) / (float)frameLength) * incValues[i]);
  }
}

songInfo download_song_info(){
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if(curl){
    curl_easy_setopt(curl, CURLOPT_URL, "https://icecast.laspegas.us/status-json.xsl");
    std::string readBuffer;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: laspegas.us progress bar");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    //std::cout << readBuffer << std::endl;

    std::regex title_parser("(([0-9]+)::([0-9]+)::([0-9]+))");
    std::smatch m;
    std::regex_search(readBuffer, m, title_parser);

    songInfo song;
    song.id = std::stoi(m[2]);
    song.start_timestamp = std::stoi(m[3]);
    song.length = std::stoi(m[4]);

    std::cout << "id: " << m[2] << " start timestamp: " << m[3] << " length: " << m[4] << std::endl;

    curl_global_cleanup();
    return song;
  }
}

void update_progress(){
  int currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  songInfo song = download_song_info();
  if(song.id == currentSongId)
    return;

  int timeElapsed = currentTimestamp - song.start_timestamp;
  currentSongId = song.id;
  totalTime = song.length;
  currentFrame = timeElapsed * FPS;
  currentPos = 1.0 * ((float)currentFrame) / (totalTime * (float)FPS);
}

void reshape(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, 1, 0, 1);
}


void display() {
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  //newColor();
  //std::cout << "r: " << currentColor[0] << " g: " << currentColor[1] << " b: " << currentColor[2] << std::endl;
  glColor3f(currentColor[0], currentColor[1], currentColor[2]);
  glRectf((GLfloat)0.0, (GLfloat)0.0, (GLfloat)currentPos, (GLfloat)1);
  glFlush();
  glutSwapBuffers();
}

void timer(int v) {

  currentFrame += 1;
  if (currentPos >= 1.0) {
    //currentFrame = 0;
    currentPos = 1.0;
  }

  else{
    currentPos = 1.0 * ((float)currentFrame) / (totalTime * (float)FPS);
  }

  if(!currentSongId || (currentPos > 0.95) && !(currentFrame % 100)) {
    //std::cout << "Pos: " << std::to_string(currentPos) << ", totalFPS: " << std::to_string(totalTime * FPS) << ", currentFrame: " << std::to_string(currentFrame) << std::endl;
    update_progress();
  }

  glutPostRedisplay();
  glutTimerFunc(1000/FPS, timer, v);
}

int main(int argc, char** argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  //glutInitWindowPosition(80, 80);
  glutInitWindowSize(width, height);
  glutCreateWindow("Progress Bar");
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutTimerFunc((int)(1000/FPS), timer, 0);
  glutMainLoop();
}