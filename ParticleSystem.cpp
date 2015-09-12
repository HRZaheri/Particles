
#include "stdafx.h"
#include "ParticleSystem.h"
#include "GLHelpers.h"
#include "gtc/matrix_transform.hpp"
#include "Defs.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <time.h>
#include <opencv/cv.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>


// --------------------------------------------------
//  constants
// --------------------------------------------------
#ifndef M_PI
#define M_PI 3.141592653589793
#endif

const int NUM_CUBE_VERTICES = 19;
const int NUM_CUBE_ELEMENTS = 12;
const int NUM_BOX_ELEMENTS  = 12;

const int NUM_SPHERE_RES_THETA = 256;
const int NUM_SPHERE_RES_PHI   = 128;
const int NUM_SPHERE_ELEMENTS  = NUM_SPHERE_RES_PHI * (NUM_SPHERE_RES_THETA * 6 + 2 * 3);
const float RADIUS_SPHERE = 0.5f;
const int sphere_num_vertices = NUM_SPHERE_RES_THETA * NUM_SPHERE_RES_PHI * 8;
const int sphere_num_textures = NUM_SPHERE_RES_THETA * NUM_SPHERE_RES_PHI * 4;
const int sphere_num_indices = NUM_SPHERE_RES_THETA * NUM_SPHERE_RES_PHI * 6;

const int NUM_TORUS_RES_P    = 256;
const int NUM_TORUS_RES_T    = 512;
const int NUM_TORUS_ELEMENTS = NUM_TORUS_RES_P * NUM_TORUS_RES_T * 6;
const float RADIUS_TORUS_IN  = 0.16f;
const float RADIUS_TORUS_OUT = 0.34f;
const int torus_num_vertices = NUM_TORUS_RES_P * NUM_TORUS_RES_T * 4;
const int torus_num_texture = NUM_TORUS_RES_P * NUM_TORUS_RES_T * 2;
const int torus_num_indices = NUM_TORUS_RES_P * NUM_TORUS_RES_T * 6;


/**
 * @brief ParticleSystem constructor
 */
ParticleSystem::ParticleSystem(COGL4CoreAPI *Api)
    : RenderPlugin(Api), wWidth(1000), wHeight(1000) {
  this->myName = "PCVC/ParticleSystem";
  this->myDescription = "Fifth assignment of PCVC";
//  oldMousePosition = glm::vec2(-1, -1);
}

/**
 * @brief ParticleSystem destructor
 */
ParticleSystem::~ParticleSystem() {}


/**
 * @brief ParticleSystem activate method
 */
bool ParticleSystem::Activate(void) {
  // --------------------------------------------------
  //  Get the path name of the plugin folder, e.g.
  //  "/path/to/oglcore/Plugins/PCVC/ParticleSystem"
  // --------------------------------------------------
  std::string pathName = this->GetCurrentPluginPath();

  // --------------------------------------------------
  //  Initialize manipulator for camera view
  // --------------------------------------------------
  int camHandle = this->AddManipulator("View", &this->viewMX,
                                       Manipulator::MANIPULATOR_ORBIT_VIEW_3D);
  this->SelectCurrentManipulator(camHandle);
  this->SetManipulatorRotation(camHandle, glm::vec3(0, 1, 0), 180.0f);
  this->SetManipulatorRotation(camHandle, glm::vec3(1, 0, 0), 180.0f);
  this->SetManipulatorDolly(camHandle, -1.5f);
  modelMX = glm::scale(glm::translate(glm::mat4(), glm::vec3(0.0f, -0.25f, 0.0f)), glm::vec3(0.5f, 0.5f, 0.5f));

  // --------------------------------------------------
  //  Initialize API variables here
  // --------------------------------------------------
  fovY.Set(this, "FoVy");
  fovY.Register();
  fovY.SetMinMax(5.0f, 90.0f);
  fovY = 45.0f;

  simulationSpeed.Set(this, "simulationSpeed");
  simulationSpeed.Register("step=0.01 min=0.01 max=1.0");
  simulationSpeed = 0.2f;

  num_points.Set(this, "numOfPoints", &OnnumPointsChanged);
  num_points.Register("step=1000 min=1 max=500000");
  num_points = 100000;

  num_instances.Set(this, "num_instances");
  num_instances.Register("step=1 min=1 max=3");
  num_instances = 1;

  pointSize.Set(this, "pointSize");
  pointSize.Register("step=0.1 min=0.1 max=10.0");
  pointSize = 0.1f;

  scale.Set(this, "scale");
  scale.Register("step=0.01 min=0.0 max=2.0");
  scale = 0.5f;

  velocity.Set(this, "velocity", &OnColorChanged);
  velocity.Register();
  velocity = glm::vec3(0.01f, 0.01f, 0.01f);
  targetSpeed = velocity;

  showBox.Set(this, "showBox");
  showBox.Register();
  showBox = true;

  stopMotion.Set(this, "stopMotion");
  stopMotion.Register();
  stopMotion = false;

  EnumPair ep[] = {{0, "None"},{1, "cube"}, {2, "sphere"}, {3, "torus"}, {4, "ef"}, {5, "ef5"}};
  importObject.Set(this, "importObject", ep, 6, &onObjectChanged);
  importObject.Register();

  // --------------------------------------------------
  //  Initialize shaders and VAs
  // --------------------------------------------------
  vsQuad = pathName + std::string("/resources/quad.vert");
  fsQuad = pathName + std::string("/resources/quad.frag");
  shaderQuad.CreateProgramFromFile(vsQuad.c_str(),
                                   fsQuad.c_str());
  vaQuad.Create(4);
  vaQuad.SetArrayBuffer(0, GL_FLOAT, 2, ogl4_2dQuadVerts);

  vsBox = pathName + std::string("/resources/box.vert");
  fsBox = pathName + std::string("/resources/box.frag");
  shaderBox.CreateProgramFromFile(vsBox.c_str(),
                                  fsBox.c_str());

  vsCube = pathName + std::string("/resources/cube.vert");
  gsCube = pathName + std::string("/resources/cube.geom");
  fsCube = pathName + std::string("/resources/cube.frag");
  shaderCube.CreateProgramFromFile( vsCube.c_str(), gsCube.c_str(), fsCube.c_str() );
  createCube();

  vsObject = pathName + std::string("/resources/object.vert");
  gsObject = pathName + std::string("/resources/object.geom");
  fsObject = pathName + std::string("/resources/object.frag");
  shaderObject.CreateProgramFromFile( vsObject.c_str(), fsObject.c_str() );//gsObject.c_str(),

  createTransformProgram(pathName);
  createBox();
  createSphere();
  createTorus();
  createCube();
  createEF(pathName, true);
  createEF(pathName, false);


  if(!checkProgram(pathName, "/resources/ctrlPoints.vert",
                   "/resources/ctrlPoints.frag")){//, "/resources/ctrlPoints.geom"
      std::cout.flush();
      std::cout << "******** shader error ********" <<std::endl;
      exit(0);
  }
  createCtrlPoints();
  vsCtrlPoints = pathName + std::string("/resources/ctrlPoints.vert");
  fsCtrlPoints = pathName + std::string("/resources/ctrlPoints.frag");
  shaderCtrlPoints.CreateProgramFromFile(vsCtrlPoints.c_str(),//gsCtrlPoints.c_str(),
                                         fsCtrlPoints.c_str());


  // --------------------------------------------------
  //  fbo and textures
  // --------------------------------------------------
  initFBO();

  // --------------------------------------------------
  //  random textures
  // --------------------------------------------------
  srand (time(NULL));
  for(int i = 0; i < random_tex_size; i++) {
      for(int j = 0; j < random_tex_size; j++) {
          texdata[4 * (i * random_tex_size + j) + 0] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));// - 0.5f;
          texdata[4 * (i * random_tex_size + j) + 1] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));// - 0.5f;
          texdata[4 * (i * random_tex_size + j) + 2] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));// - 0.5f;
          texdata[4 * (i * random_tex_size + j) + 3] = 1.0f;
      }
  }

  glGenTextures(1, &randomTex);
  glBindTexture(GL_TEXTURE_2D, randomTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, random_tex_size, random_tex_size, 0, GL_RGBA, GL_FLOAT, texdata);

  int tex_cube_width = 800;
  int tex_cube_height = 400;
  char *texpathBoard = (pathName + std::string("/resources/textures/board.ppm")).c_str();
  texBoard = LoadPPMTexture(texpathBoard, tex_cube_width, tex_cube_height);

  //target point
  targetPoint= glm::vec3(static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
                         static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
                         static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
  direction = glm::vec3(1.0f,1.0f,1.0f);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClearDepth(1.0f);
  glEnable(GL_DEPTH_TEST);

  return true;
}

/**
 * @brief ParticleSystem deactive method
 */
bool ParticleSystem::Deactivate(void) {
    // --------------------------------------------------
    //  Remove shaders and VAs
    // --------------------------------------------------
  shaderQuad.RemoveAllShaders();
  shaderCube.RemoveAllShaders();
  shaderBox.RemoveAllShaders();
  shaderCtrlPoints.RemoveAllShaders();
  shaderObject.RemoveAllShaders();

  vaQuad.Delete();
  vaCube.Delete();
  vaBox.Delete();

  glDisable(GL_DEPTH_TEST);
  return true;
}

/**
 * @brief ParticleSystem initialization method
 */
bool ParticleSystem::Init(void) {
  if (gl3wInit()) {
    fprintf(stderr, "Error: Failed to initialize gl3w.\n");
    return false;
  }
  return true;
}

/**
 * @brief ParticleSystem keyboard callback function
 * @param key
 * @param x
 * @param y
 */
bool ParticleSystem::Keyboard(unsigned char key, int x, int y) {
  PostRedisplay();
  return true;
}

/**
 * @brief ParticleSystem mouse motion callback function
 * @param x
 * @param y
 */
bool ParticleSystem::Motion(int x, int y) {
  return false;
}

/**
 * @brief ParticleSystem mouse callback function
 * @param button
 * @param state
 * @param x
 * @param y
 */
bool ParticleSystem::Mouse(int button, int state, int x, int y) {

  PostRedisplay();
  return false;
}

/**
 * @brief ParticleSystem Render
 */
bool ParticleSystem::Render(void) {
  viewAspect = wWidth / static_cast<float>(wHeight);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  projMX =
      glm::perspective(static_cast<float>(fovY), viewAspect, 0.01f, 100.0f);

  // --------------------------------------------------
  //  draw to FBO
  // --------------------------------------------------
  enable_renderbuffers();
  drawToFBO();
  disable_renderbuffers();

  // --------------------------------------------------
  //  draw fullscreen quad
  // --------------------------------------------------
  drawQuad();

  PostRedisplay();
  return false;
}

/**
 * @brief ParticleSystem resize method
 * @param width
 * @param height
 */
bool ParticleSystem::Resize(int width, int height) {
  wWidth = width;
  wHeight = height;
  viewAspect = wWidth / static_cast<float>(wHeight);

  initFBO();
  Render();

  return true;
}

void ParticleSystem::OnColorChanged(APIVar<ParticleSystem, Color3FVarPolicy> &var)
{
    targetSpeed = velocity;
}

void ParticleSystem::OnnumPointsChanged(APIVar<ParticleSystem, IntVarPolicy> &var)
{
    createCtrlPoints();
}

void ParticleSystem::onObjectChanged(EnumVar<ParticleSystem> &var)
{
    objectIndex = int(var);
    tex_counter = 0;
}

/**
 * @brief Initialize framebuffer object
 */
bool ParticleSystem::initFBO() {

  colAttachID[0] = initTex2D(wWidth, wHeight);

  glGenRenderbuffers(1, &dboID);
  glBindRenderbuffer(GL_RENDERBUFFER, dboID);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, wWidth, wHeight);

  glGenFramebuffers(1, &fboID);
  glBindFramebuffer(GL_FRAMEBUFFER, fboID);

  glActiveTexture(GL_TEXTURE0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         colAttachID[0], 0);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, dboID);

  checkFramebufferStatus();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return false;
}

/**
 * @brief Draw to framebuffer object
 */
void ParticleSystem::drawToFBO() {

    glViewport(0,0, wWidth,wHeight);
    projMX = glm::perspective((float)this->fovY, viewAspect, 0.01f,100.0f );

    GLenum drawBuffer[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffer);
    glBindTexture(GL_TEXTURE_2D, colAttachID[0]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    drawBox();
    drawPoints();
    drawObject();
}

/************************************ create objects *************************************/

/**
 *  @brief
 */
void ParticleSystem::createEF(std::string pathName, bool first){
    cv::Mat ef_img;
    if(first)
        ef_img = cv::imread(pathName + std::string("/resources/textures/ef.jpg"),
                                CV_LOAD_IMAGE_GRAYSCALE);
    else

        ef_img = cv::imread(pathName + std::string("/resources/textures/ef5.jpg"),
                                CV_LOAD_IMAGE_GRAYSCALE);
    cv::resize(ef_img,ef_img,cv::Size(100,100));
    std::vector<float> pixels;

    int r = ef_img.rows;
    int c = ef_img.cols;
    std::cout << ef_img.type()<< std::endl;
    std::cout << ef_img.channels()<< std::endl;

    for(int i=0; i<r; i++){
        for(int j=0; j<c; j++){
//            std::cout << (int)ef_img.at<char>(i,j)<< std::endl;
            if((int)ef_img.at<uchar>(j,i)>250){
                pixels.push_back((float)i/r-0.5);
//                std::cout << (float)i/r-0.5 << std::endl;
                pixels.push_back((float)j/c-0.5);
                pixels.push_back(0.0f);
                pixels.push_back(1.0f);
            }
        }
    }
    std::cout << "size : " << pixels.size()/4 << std::endl;
    int s = 10000;
    NUM_EF_VERTICES = pixels.size()/4;//

    float efTexture[pixels.size()];//pixels.size()
//    float *efTexture = &pixels[0];
    for(int i=0; i<pixels.size(); i++){//
        if(i%4==0)
            coeff = 0.35;
        else if(i%4==2)
            coeff = 0.5;
        else
            coeff = 0.6;
        if(efTexture[i]!=1.0f)
            efTexture[i] = (pixels[i] + 0.5f) * 1.5 + coeff;
        else
            efTexture[i] = pixels[i];
//        std::cout << efTexture[i] << std::endl;
    }
    // --------------------------------------------------
    //  EF texture
    // --------------------------------------------------
    if(first){
        glGenTextures(1, &tex_ef);
        glBindTexture(GL_TEXTURE_2D, tex_ef);
    }
    else{
        glGenTextures(1, &tex_ef5);
        glBindTexture(GL_TEXTURE_2D, tex_ef5);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NUM_EF_VERTICES, 1,
                 0, GL_RGBA, GL_FLOAT, efTexture);
}

/**
 *  @brief Create vertex array object for cube.
 */
void ParticleSystem::createCube() {
    float cubeVertices[] = {
            -0.5f, -0.5f, -0.5f, 1.0f,
             0.5f, -0.5f, -0.5f, 1.0f,
            -0.5f, -0.5f, -0.5f, 1.0f,
            -0.5f, -0.5f,  0.5f, 1.0f,
             0.5f, -0.5f,  0.5f, 1.0f,
             0.5f, -0.5f, -0.5f, 1.0f,
            -0.5f, -0.5f, -0.5f, 1.0f,
            -0.5f,  0.5f, -0.5f, 1.0f,
            -0.5f,  0.5f,  0.5f, 1.0f,
             0.5f,  0.5f,  0.5f, 1.0f,
             0.5f,  0.5f, -0.5f, 1.0f,
            -0.5f,  0.5f, -0.5f, 1.0f,
            -0.5f,  0.5f, -0.5f, 1.0f,
             0.5f,  0.5f, -0.5f, 1.0f
        };

    float boxVertices[] = {
        +0.5f, +0.5f, -0.5f, 1.0f,//0
        -0.5f, +0.5f, -0.5f, 1.0f,//1
        -0.5f, +0.5f, +0.5f, 1.0f,//2
        +0.5f, +0.5f, +0.5f, 1.0f,//3
        +0.5f, +0.5f, -0.5f, 1.0f,//4
        -0.5f, +0.5f, +0.5f, 1.0f,//5
        -0.5f, -0.5f, +0.5f, 1.0f,//6
        +0.5f, -0.5f, +0.5f, 1.0f,//7
        +0.5f, +0.5f, +0.5f, 1.0f,//8
        -0.5f, -0.5f, +0.5f, 1.0f,//9
        -0.5f, -0.5f, -0.5f, 1.0f,//10
        -0.5f, +0.5f, -0.5f, 1.0f,//11
        -0.5f, -0.5f, +0.5f, 1.0f,//12
        +0.5f, -0.5f, -0.5f, 1.0f,//13
        +0.5f, -0.5f, +0.5f, 1.0f,//14
        +0.5f, +0.5f, -0.5f, 1.0f,//15
        +0.5f, -0.5f, -0.5f, 1.0f,//16
        -0.5f, -0.5f, -0.5f, 1.0f,//17
        +0.5f, +0.5f, -0.5f, 1.0f,//18
    };

    float coeff;
    float cubeTexture[NUM_CUBE_VERTICES*4];
    for(int i=0; i<NUM_CUBE_VERTICES*4; i++){
        if(i%4==0)
            coeff = 0.35;
        else if(i%4==2)
            coeff = 0.5;
        else
            coeff = 0.6;
        if(boxVertices[i]!=1.0f)
            cubeTexture[i] = (boxVertices[i] + 0.5f) * 1.5 + coeff;
        else
            cubeTexture[i] = boxVertices[i];
//        std::cout << cubeTexture[i] << std::endl;
    }

        GLuint cubeFaces[NUM_CUBE_VERTICES*2];
        for(int i=0; i < NUM_CUBE_VERTICES; i++){
            cubeFaces[2*i+0] = i;
            cubeFaces[2*i+1] = i+1;
        }

        vaCube.Create(NUM_CUBE_VERTICES);
        vaCube.SetArrayBuffer(0,GL_FLOAT,4,boxVertices);
        vaCube.SetElementBuffer(0, NUM_BOX_ELEMENTS*3, cubeFaces, GL_STATIC_DRAW);


        // --------------------------------------------------
        //  cube texture for replication
        // --------------------------------------------------
        glGenTextures(1, &tex_cube);
        glBindTexture(GL_TEXTURE_2D, tex_cube);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NUM_CUBE_VERTICES, 1,
                     0, GL_RGBA, GL_FLOAT, cubeTexture);

}

void ParticleSystem::createSphere()
{

    float theta_step = 2.0 * M_PI / NUM_SPHERE_RES_THETA;
    float phi_step =  M_PI / NUM_SPHERE_RES_PHI;
    float sphereVertices[sphere_num_vertices/2];
    int offset;

    for(int p=0 ; p< NUM_SPHERE_RES_PHI; p++){
        for(int t=0 ; t< NUM_SPHERE_RES_THETA; t++){
            sphereVertices[4 * (p * NUM_SPHERE_RES_THETA + t) + 0] = RADIUS_SPHERE * cos(-p * phi_step) * cos( -t * theta_step);
            sphereVertices[4 * (p * NUM_SPHERE_RES_THETA + t) + 1] = RADIUS_SPHERE * cos(-p * phi_step) * sin( -t * theta_step);
            sphereVertices[4 * (p * NUM_SPHERE_RES_THETA + t) + 2] = RADIUS_SPHERE * sin(-p * phi_step);
            sphereVertices[4 * (p * NUM_SPHERE_RES_THETA + t) + 3] = 1.0f;
            offset = p * NUM_SPHERE_RES_THETA + t;
        }
    }

    offset++;

    float sphereVertices_1[sphere_num_vertices/2];

    for(int p=0 ; p< NUM_SPHERE_RES_PHI; p++){
        for(int t=0 ; t< NUM_SPHERE_RES_THETA; t++){
            sphereVertices_1[4 * (p * NUM_SPHERE_RES_THETA + t) + 0] = RADIUS_SPHERE * cos(p * phi_step) * cos( t * theta_step);
            sphereVertices_1[4 * (p * NUM_SPHERE_RES_THETA + t) + 1] = RADIUS_SPHERE * cos(p * phi_step) * sin( t * theta_step);
            sphereVertices_1[4 * (p * NUM_SPHERE_RES_THETA + t) + 2] = RADIUS_SPHERE * sin(p * phi_step);
            sphereVertices_1[4 * (p * NUM_SPHERE_RES_THETA + t) + 3] = 1.0f;
        }
    }

    GLuint sphereIndices[sphere_num_indices/2];
    int k=0;
    int i=0;
    for(int p=0 ; p< NUM_SPHERE_RES_PHI; p++){
        for(int t=0 ; t< NUM_SPHERE_RES_THETA; t++){
            if(!(k%2)){
                sphereIndices[3*k + 0] = i;
                sphereIndices[3*k + 1] = (i + NUM_SPHERE_RES_THETA) ;
                sphereIndices[3*k + 2] = (i + NUM_SPHERE_RES_THETA-1);

            }
            else{
                sphereIndices[3*k + 0] = i;
                sphereIndices[3*k + 1] = i+1;
                sphereIndices[3*k + 2] = (i + NUM_SPHERE_RES_THETA);
                i += 1 ;
            }
            k += 1 ;
        }
    }

    float sphereVertices_all [sphere_num_vertices];
    for(int i=0; i < sphere_num_vertices; i++ ){
        if(i<sphere_num_vertices/2)
            sphereVertices_all[i] = sphereVertices[i];
        else
            sphereVertices_all[i] = sphereVertices_1[i-(sphere_num_vertices/2)];
    }

    GLuint sphereIndices_all[sphere_num_indices];
    for(int i=0; i < sphere_num_indices; i++ ){
        if(i<sphere_num_indices/2)
            sphereIndices_all[i] = sphereIndices[i];
        else
            sphereIndices_all[i] = sphereIndices[i-(sphere_num_indices/2)] + offset;
    }


    glGenBuffers(1, &vbo_sphere);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_sphere);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphereVertices_all), sphereVertices_all, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo_sphere);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_sphere);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sphereIndices_all), sphereIndices_all, GL_STATIC_DRAW);

    glGenVertexArrays(1, &va_sphere);
    glBindVertexArray(va_sphere);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_sphere);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0,0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_sphere);
    glBindVertexArray(0);

    float coeff;
    float sphereTexture[sphere_num_vertices];
    for(int i=0; i<sphere_num_vertices; i++){
        if(i%4==0)
            coeff = 0.35;
        else if(i%4==2)
            coeff = 0.5;
        else
            coeff = 0.6;
        if(sphereVertices_all[i]!=1.0f)
            sphereTexture[i] = (sphereVertices_all[i] + 0.5f) * 1.5 + coeff;
        else
            sphereTexture[i] = sphereVertices_all[i];
    }

    // --------------------------------------------------
    //  sphere texture for replication
    // --------------------------------------------------
    glGenTextures(1, &tex_sphere);
    glBindTexture(GL_TEXTURE_2D, tex_sphere);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NUM_SPHERE_RES_THETA, NUM_SPHERE_RES_PHI*2,
                 0, GL_RGBA, GL_FLOAT, sphereTexture);
}

void ParticleSystem::createTorus() {

    float p_step = 2.0 * M_PI / NUM_TORUS_RES_P;
    float t_step = 2.0 * M_PI / NUM_TORUS_RES_T;
    float torusVertices[torus_num_vertices];
    int offset;
    for(int t=0 ; t< NUM_TORUS_RES_T; t++){
        for(int p=0 ; p< NUM_TORUS_RES_P; p++){
            torusVertices[4 * (t * NUM_TORUS_RES_P + p) + 0] =
                    (RADIUS_TORUS_OUT + RADIUS_TORUS_IN * cos(p * p_step)) *cos(t * t_step);
            torusVertices[4 * (t * NUM_TORUS_RES_P + p) + 1] =
                    (RADIUS_TORUS_OUT + RADIUS_TORUS_IN * cos(p * p_step)) *sin(t * t_step);
            torusVertices[4 * (t * NUM_TORUS_RES_P + p) + 2] = RADIUS_TORUS_IN * sin(p * p_step);
            torusVertices[4 * (t * NUM_TORUS_RES_P + p) + 3] = 1.0f;
            offset = t * NUM_TORUS_RES_P + p;
        }
    }


    GLuint torusIndices[NUM_TORUS_ELEMENTS];
    int i=0;
//    int k=0;
//    while(k<NUM_TORUS_RES_P * (NUM_TORUS_RES_T-1) * 2 ){
    int up_bound = NUM_TORUS_RES_P * NUM_TORUS_RES_T * 2;
    for(int k=0; k < up_bound; k++){
        if(!(k%2)){
            torusIndices[3*k + 0] = i;
            torusIndices[3*k + 1] = (i + NUM_TORUS_RES_P)%(offset - NUM_TORUS_RES_P);
            torusIndices[3*k + 2] = (i + NUM_TORUS_RES_P-1)%(offset - NUM_TORUS_RES_P);

        }
        else{
            torusIndices[3*k + 0] = i;
            torusIndices[3*k + 1] = i+1;
            torusIndices[3*k + 2] = (i + NUM_TORUS_RES_P)%(offset - NUM_TORUS_RES_P);
            i += 1 ;
        }
//        k += 1 ;

//        if (i > (NUM_TORUS_RES_P-1))
//            break;
    }

    glGenBuffers(1, &vbo_torus);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_torus);
    glBufferData(GL_ARRAY_BUFFER, sizeof(torusVertices), torusVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo_torus);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_torus);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(torusIndices), torusIndices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &va_torus);
    glBindVertexArray(va_torus);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_torus);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0,0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_torus);
    glBindVertexArray(0);

    float coeff;
    float torusTexture[NUM_TORUS_RES_P*NUM_TORUS_RES_T*4];
    for(int i=0; i<NUM_TORUS_RES_P*NUM_TORUS_RES_T*4; i++){
        if(i%4==0)
            coeff = 0.35;
        else if(i%4==2)
            coeff = 0.5;
        else
            coeff = 0.6;
        if(torusVertices[i]!=1.0f)
            torusTexture[i] = (torusVertices[i] + 0.5f)* 1.5 + coeff;
        else
            torusTexture[i] = torusVertices[i];

//        std::cout << torusTexture[i] << ", ";
    }


    // --------------------------------------------------
    //  torus texture for replication
    // --------------------------------------------------
    glGenTextures(1, &tex_torus);
    glBindTexture(GL_TEXTURE_2D, tex_torus);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NUM_TORUS_RES_T, NUM_TORUS_RES_P,
                 0, GL_RGBA, GL_FLOAT, torusTexture);
}

/**
 * @brief initialize control points
 */
void ParticleSystem::createCtrlPoints(){
    float points_position[num_points * 4];
    float points_ID[(int)num_points];
    srand (time(NULL));
    for(int i = 0; i < num_points; i++) {
          points_ID[i] = (float)i;

          float j = (float)(i%360);
          points_position[4*i+0] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
          points_position[4*i+1] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
          points_position[4*i+2] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
          points_position[4*i+3] = 1.0f;
    }

//    srand (time(NULL));
    float points_speed[num_points * 3];
    for(int i = 0; i < num_points; i++) {
        float j = (float)i;
          points_speed[3*i+0] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)-0.5f) * j/num_points;
          points_speed[3*i+1] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)-0.5f) * j/num_points;
          points_speed[3*i+2] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)-0.5f) * j/num_points;
    }

    glGenBuffers(1, &vbo_position);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points_position), points_position, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &vbo_ID);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points_ID), points_ID, GL_STATIC_DRAW);

    glGenBuffers(1, &vbo_speed);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_speed);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points_speed), points_speed,GL_STATIC_DRAW);


    glGenVertexArrays(1, &va_points);
    glBindVertexArray(va_points);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0,0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_speed);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_ID);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0,0);
    glBindVertexArray(0);


    glGenBuffers(1, &vbo_draw);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_draw);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points_position), NULL, GL_DYNAMIC_COPY);

    glGenBuffers(1, &vbo_draw_speed);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_draw_speed);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points_speed), NULL,GL_DYNAMIC_COPY);

    glGenVertexArrays(1, &va_draw);
    glBindVertexArray(va_draw);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_draw);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0,0);
    glBindVertexArray(0);
}

void ParticleSystem::createBox()
{
    const float ogl4_4dBoxVerts[] = {
      0.0f,0.0f*0.5f+0.5f,0.0f,1.0f,
      1.0f,0.0f*0.5f+0.5f,0.0f,1.0f,
      1.0f,1.0f*0.5f+0.5f,0.0f,1.0f,
      0.0f,1.0f*0.5f+0.5f,0.0f,1.0f,
      0.0f,0.0f*0.5f+0.5f,1.0f,1.0f,
      1.0f,0.0f*0.5f+0.5f,1.0f,1.0f,
      1.0f,1.0f*0.5f+0.5f,1.0f,1.0f,
      0.0f,1.0f*0.5f+0.5f,1.0f,1.0f
    };

    //! Standard box edges IDs.
    const unsigned int ogl4_BoxEdges[] = {
      0,1, 1,2, 2,3, 3,0,
      4,5, 5,6, 6,7, 7,4,
      0,4, 1,5, 2,6, 3,7
    };

      vaBox.Create(ogl4_num4dBoxVerts);
      vaBox.SetArrayBuffer(0, GL_FLOAT, 4, ogl4_4dBoxVerts);
      vaBox.SetElementBuffer(0, ogl4_numBoxEdges * 2, ogl4_BoxEdges);

}

/************************************ compile shaders *************************************/

void ParticleSystem::createTransformProgram(std::string pathName){
    std::string vertexPath = std::string("/resources/transform.vert");
//    shaderTransform.CreateProgramFromFile(vsTransform.c_str());
    GLuint vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* adapter[1];

    std::string temp = readShaderCode((pathName + vertexPath).c_str());
    adapter[0] = temp.c_str();
    glShaderSource(vertexShader, 1, adapter, 0);

    glCompileShader(vertexShader);

    if(!checkShaderStatus(vertexShader)){
        std::cout.flush();
        std::cout << "******** shader error ********" <<std::endl;
        exit(0);
    }

    program_transform = glCreateProgram();
    glProgramParameteri(program_transform, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(program_transform, vertexShader);

    //feedback transform
    const GLchar* feedbackVaryings[] = { "Position"};
    glTransformFeedbackVaryings(program_transform, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);

    glLinkProgram(program_transform);
    if(!checkPogramStatus(program_transform)){
        std::cout.flush();
        std::cout << "******** shader error ********" <<std::endl;
        exit(0);
    }

    return true;
}

bool ParticleSystem::checkProgram(std::string pathName, std::string vertexPath, std::string fragPath){
        GLuint shaderProg, vertexShader, fragmentShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar* adapter[1];

        std::string temp = readShaderCode((pathName + vertexPath).c_str());
        adapter[0] = temp.c_str();
        glShaderSource(vertexShader, 1, adapter, 0);

        temp = readShaderCode((pathName + fragPath).c_str());
        adapter[0] = temp.c_str();
        glShaderSource(fragmentShader, 1, adapter, 0);

        glCompileShader(vertexShader);
        glCompileShader(fragmentShader);

        if(!checkShaderStatus(vertexShader) || !checkShaderStatus(fragmentShader))
            return false;

        shaderProg = glCreateProgram();
        glAttachShader(shaderProg, vertexShader);
        glAttachShader(shaderProg, fragmentShader);
        glLinkProgram(shaderProg);
        if(!checkPogramStatus(shaderProg))
            return false;

        return true;
}

bool ParticleSystem::checkProgram(std::string pathName, std::string vertexPath, std::string fragPath, std::string geomPath){
        GLuint shaderProg, vertexShader, fragmentShader, geometryShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
        const GLchar* adapter[1];

        std::string temp = readShaderCode((pathName + vertexPath).c_str());
        adapter[0] = temp.c_str();
        glShaderSource(vertexShader, 1, adapter, 0);

        temp = readShaderCode((pathName + fragPath).c_str());
        adapter[0] = temp.c_str();
        glShaderSource(fragmentShader, 1, adapter, 0);


        temp = readShaderCode((pathName + geomPath).c_str());
        adapter[0] = temp.c_str();
        glShaderSource(geometryShader, 1, adapter, 0);

        glCompileShader(vertexShader);
        glCompileShader(fragmentShader);
        glCompileShader(geometryShader);

        if(!checkShaderStatus(vertexShader) || !checkShaderStatus(fragmentShader)|| !checkShaderStatus(geometryShader))
            return false;

        shaderProg = glCreateProgram();
        glAttachShader(shaderProg, vertexShader);
        glAttachShader(shaderProg, geometryShader);
        glAttachShader(shaderProg, fragmentShader);
        glLinkProgram(shaderProg);
        if(!checkPogramStatus(shaderProg))
            return false;

        return true;
}

bool ParticleSystem::checkShaderStatus(GLuint shaderID){
    GLint compileStatus;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
    if(compileStatus != GL_TRUE){
        GLint infoLogLength;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar* buffer = new GLchar[infoLogLength];

        GLsizei bufferSize;
        glGetShaderInfoLog(shaderID, infoLogLength, &bufferSize, buffer);
        std::cout<< buffer << std::endl;
        delete [] buffer;
        return false;
    }
    return true;
}

bool ParticleSystem::checkPogramStatus(GLuint programID){
    GLint linkStatus;
    glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus);
    if(linkStatus != GL_TRUE){
        GLint infoLogLength;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar* buffer = new GLchar[infoLogLength];

        GLsizei bufferSize;
        glGetProgramInfoLog(programID, infoLogLength, &bufferSize, buffer);
        std::cout<< buffer << std::endl;
        delete [] buffer;
        return false;
    }
    return true;
}

/**
 * @brief read shader code from resource repos.
 */
std::string ParticleSystem::readShaderCode(const char* fileName){
    std::ifstream InputFile(fileName);
    if(!InputFile.good()){
        std::cout<<"file failed to load..."<<fileName<<std::endl;
        exit(1);
    }
    return std::string(
            std::istreambuf_iterator<char>(InputFile),
            std::istreambuf_iterator<char>());
}

/**
 * @brief Check status of bound framebuffer object (FBO).
 */
void ParticleSystem::checkFramebufferStatus() {
    GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
    switch (status) {
        case GL_FRAMEBUFFER_UNDEFINED: {
            fprintf(stderr,"FBO: undefined.\n");
            break;
        }
        case GL_FRAMEBUFFER_COMPLETE: {
            fprintf(stderr,"FBO: complete.\n");
            break;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: {
            fprintf(stderr,"FBO: incomplete attachment.\n");
            break;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: {
            fprintf(stderr,"FBO: no buffers are attached to the FBO.\n");
            break;
        }
        case GL_FRAMEBUFFER_UNSUPPORTED: {
            fprintf(stderr,"FBO: combination of internal buffer formats is not supported.\n");
            break;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: {
            fprintf(stderr,"FBO: number of samples or the value for ... does not match.\n");
            break;
        }
    }
}

/************************************ rendering *************************************/

/**
 * @brief draw a full screen quad
 */
void ParticleSystem::drawQuad() {
  glViewport(0,0,wWidth,wHeight);
  glm::mat4 pmx = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);
  shaderQuad.Bind();
  glUniformMatrix4fv(shaderQuad.GetUniformLocation("projMX"), 1, GL_FALSE,
                     glm::value_ptr(pmx));
  glEnable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, colAttachID[0]);
  glUniform1i(shaderQuad.GetUniformLocation("tex"), 0);

  vaQuad.Bind();
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  vaQuad.Release();

  shaderQuad.Release();
}

/**
 * @brief draw Box
 */
void ParticleSystem::drawBox() {
   if(!showBox)
       return;
  shaderBox.Bind();
  glUniformMatrix4fv(shaderBox.GetUniformLocation("projMX"), 1, GL_FALSE,
                     glm::value_ptr(projMX));
  glUniformMatrix4fv(shaderBox.GetUniformLocation("viewMX"), 1, GL_FALSE,
                     glm::value_ptr(viewMX));
  glUniform3f(shaderBox.GetUniformLocation("translate"), 0.5f, 0.5f, 0.5f);
  vaBox.Bind();
//  vaCube.Bind();
  glDrawElements(GL_LINES, ogl4_numBoxEdges * 2, GL_UNSIGNED_INT, 0);
  vaBox.Release();
//  vaCube.Release();
  shaderBox.Release();
}

/**
 * @brief draw the imported object
 */
void ParticleSystem::drawObject(){
    shaderObject.Bind();
        glUniformMatrix4fv(shaderObject.GetUniformLocation("projMX"), 1, GL_FALSE,
                           glm::value_ptr(projMX));
        glUniformMatrix4fv(shaderObject.GetUniformLocation("viewMX"), 1, GL_FALSE,
                           glm::value_ptr(viewMX));
//        glUniform3f(shaderObject.GetUniformLocation("translate"), 1.0f, 0.0f, 0.0f);

        if(objectIndex == 1){
            glUniformMatrix4fv(shaderObject.GetUniformLocation("modelMX"), 1, GL_FALSE,
                               glm::value_ptr(glm::rotate(modelMX, 180.0f, glm::vec3(0.0f, 0.0f, 1.0f))));
            vaCube.Bind();
            glDrawElements(GL_LINES, NUM_CUBE_VERTICES*2 ,GL_UNSIGNED_INT,(void*)0);
            vaCube.Release();
        }
        else if(objectIndex == 2 ){
            glUniformMatrix4fv(shaderObject.GetUniformLocation("modelMX"), 1, GL_FALSE,
                               glm::value_ptr(modelMX));
            glBindVertexArray(va_sphere);
            glDrawElements(GL_LINES, sphere_num_indices ,GL_UNSIGNED_INT,(void*)0);
            glBindVertexArray(0);
        }
        else if(objectIndex == 3){
            glUniformMatrix4fv(shaderObject.GetUniformLocation("modelMX"), 1, GL_FALSE,
                               glm::value_ptr(glm::rotate(modelMX, 180.0f, glm::vec3(1.0f, 0.0f, 0.0f))));

            glBindVertexArray(va_torus);
            glDrawElements(GL_LINES, (int)(NUM_TORUS_ELEMENTS-1) ,GL_UNSIGNED_INT,(void*)0);
            glBindVertexArray(0);
        }
    shaderObject.Release();
}


/**
 * @brief update particles position randomly
 */
void ParticleSystem::updateParticles(){
    targetPoint_old = targetPoint;
    targetSpeed = velocity;

    glm::vec3 val;
        if(!stopMotion){
            if(++counter % 1000 == 0.0 ){
                srand (time(NULL));
                targetSpeed += 0.001f *
                               glm::vec3(
                                    (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)),
                                    (static_cast <float> (rand()) / static_cast <float> (RAND_MAX))*0.5f+1.0f,
                                    (static_cast <float> (rand()) / static_cast <float> (RAND_MAX))
                                );
                counter = 0;
            }
        }
        else{
            targetSpeed = glm::vec3(0.0f);
        }
            val= (float)simulationSpeed * direction*targetSpeed ;
                glm::vec3 check = targetPoint + direction*targetSpeed* 0.01f;
//            glm::vec3 check = targetPoint +  glm::sin(val) * glm::cos(val);
            if (check.x > 1.0f || check.x < 0.0f ) direction.x *= (-1.0f);
            if (check.y > 1.0f || check.y < 0.5f ) {direction.y *= (-1.0f); if(check.y < 0.5f) targetPoint.y += 0.01;}
            if (check.z > 1.0f || check.z < 0.0f ) direction.z *= (-1.0f);
        targetPoint = targetPoint + (float)simulationSpeed * direction*targetSpeed;
}

/**
 * @brief set uniforms for transform feedback shader
 */
void ParticleSystem::setUniforms(){
    glUniform3f(glGetUniformLocation(program_transform,"target"), targetPoint.x, targetPoint.y, targetPoint.z);
//    std::cout << targetPoint.x << " , " <<targetPoint.y << " , " << targetPoint.z << " \n";
   glUniform1f(glGetUniformLocation(program_transform,"scale"), (float)scale);
    srand (time(NULL));
    glm::vec3 r = glm::vec3(static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
                               static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
                               static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
    glUniform2f(glGetUniformLocation(program_transform,"rand"), r.x, r.y);

    glUniform1i(glGetUniformLocation(program_transform,"offset_velocity"),offset_velocity);
    if(++offset_velocity > 9000)
        offset_velocity = 0;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, randomTex);
    glUniform1i(glGetUniformLocation(program_transform,"randomtex"), 0);

    if(objectIndex==0){
        glUniform1i(glGetUniformLocation(program_transform,"mode"), 0);
    }
    else if(objectIndex == 1){//cube
        glUniform1i(glGetUniformLocation(program_transform,"mode"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_cube);
        glUniform1i(glGetUniformLocation(program_transform,"tex"), 1);
        glUniform1i(glGetUniformLocation(program_transform,"tex_width"), NUM_CUBE_VERTICES);
        glUniform1i(glGetUniformLocation(program_transform,"tex_height"), 1);
        if(tex_counter < 1000)
            glUniform1i(glGetUniformLocation(program_transform,"counter"), tex_counter++);
        else
            glUniform1i(glGetUniformLocation(program_transform,"counter"), tex_counter);

    }
    else if(objectIndex == 2){//sphere
        pointSize = 0.1f;
        glUniform1i(glGetUniformLocation(program_transform,"mode"), 2);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_sphere);
        glUniform1i(glGetUniformLocation(program_transform,"tex"), 1);
        glUniform1i(glGetUniformLocation(program_transform,"tex_width"), NUM_SPHERE_RES_THETA);
        glUniform1i(glGetUniformLocation(program_transform,"tex_height"), NUM_SPHERE_RES_PHI*2);
        if(tex_counter < 1000)
            glUniform1i(glGetUniformLocation(program_transform,"counter"), tex_counter++);
        else
            glUniform1i(glGetUniformLocation(program_transform,"counter"), tex_counter);
    }
    else if(objectIndex == 3){//torus
        pointSize = 0.1f;
        glUniform1i(glGetUniformLocation(program_transform,"mode"), 3);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_torus);
        glUniform1i(glGetUniformLocation(program_transform,"tex"), 1);
        glUniform1i(glGetUniformLocation(program_transform,"tex_width"), NUM_TORUS_RES_T);
        glUniform1i(glGetUniformLocation(program_transform,"tex_height"), NUM_TORUS_RES_P);
        if(tex_counter < 1000)
            glUniform1i(glGetUniformLocation(program_transform,"counter"), tex_counter++);
        else
            glUniform1i(glGetUniformLocation(program_transform,"counter"), tex_counter);
    }
    else if(objectIndex == 4 || objectIndex == 5){//EF
        glActiveTexture(GL_TEXTURE1);
        if(objectIndex == 5){
            glUniform1i(glGetUniformLocation(program_transform,"mode"), 5);
            glBindTexture(GL_TEXTURE_2D, tex_ef5);
            pointSize = 0.1f;
        }
        else{
            glUniform1i(glGetUniformLocation(program_transform,"mode"), 1);
            glBindTexture(GL_TEXTURE_2D, tex_ef);
            pointSize = 0.1f;
        }
        glUniform1i(glGetUniformLocation(program_transform,"tex"), 1);
        glUniform1i(glGetUniformLocation(program_transform,"tex_width"), NUM_EF_VERTICES);
        glUniform1i(glGetUniformLocation(program_transform,"tex_height"), 1);
        if(tex_counter < 1000)
            glUniform1i(glGetUniformLocation(program_transform,"counter"), tex_counter++);
        else
            glUniform1i(glGetUniformLocation(program_transform,"counter"), tex_counter);
    }
}

/**
 * @brief draw particles
 */
void ParticleSystem::drawPoints() {
//    glEnable(GL_PROGRAM_POINT_SZE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      updateParticles();

      // --------------------------------------------------
      //  transform feedback
      // --------------------------------------------------
      glUseProgram(program_transform);
      glEnable(GL_RASTERIZER_DISCARD);
            setUniforms();

              glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
              glEnableVertexAttribArray(0);
              glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0,0);
              glBindBuffer(GL_ARRAY_BUFFER, vbo_speed);
              glEnableVertexAttribArray(1);
              glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,0);
              glBindBuffer(GL_ARRAY_BUFFER, vbo_ID);
              glEnableVertexAttribArray(2);
              glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0,0);

              glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0, vbo_draw);
              glBeginTransformFeedback(GL_POINTS);
                  glDrawArrays(GL_POINTS, 0, num_points);
              glEndTransformFeedback();

            glBindBuffer(GL_ARRAY_BUFFER, 0);


      glDisable(GL_RASTERIZER_DISCARD);
      glUseProgram(0);

      // --------------------------------------------------
      //  use the results from transformation to draw the particles
      // --------------------------------------------------
      srand (time(NULL));
      glm::vec3 r = glm::vec3(static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
                                 static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
                                 static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
      shaderCtrlPoints.Bind();
      glUniform1f(shaderCtrlPoints.GetUniformLocation("pointSize"), (float)pointSize);
      glUniform3f(shaderCtrlPoints.GetUniformLocation("rand"), r.x, r.y, r.z);
      glUniformMatrix4fv(shaderCtrlPoints.GetUniformLocation("projMX"), 1, GL_FALSE,
                         glm::value_ptr(projMX));
      glUniformMatrix4fv(shaderCtrlPoints.GetUniformLocation("viewMX"), 1, GL_FALSE,
                         glm::value_ptr(viewMX));
      glUniform3f(shaderCtrlPoints.GetUniformLocation("translate"), 0.5f, 0.5f, 0.5f);
      currentTime+=deltaT;
      if(currentTime > 1000000.0f)
          currentTime = 0.0f;
      glUniform1f(shaderCtrlPoints.GetUniformLocation("time"), currentTime);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, randomTex);
      glUniform1i(shaderCtrlPoints.GetUniformLocation("randomtex"), 0);

          glBindBuffer(GL_ARRAY_BUFFER, vbo_draw);
          glEnableVertexAttribArray(0);
          glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0,0);

          glBindBuffer(GL_ARRAY_BUFFER, vbo_speed);
          glEnableVertexAttribArray(1);
          glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,0);

          glDrawArraysInstanced(GL_POINTS, 0, num_points, num_instances);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
  shaderCtrlPoints.Release();

  // --------------------------------------------------
  //  swap buffers for particles position (ping-pong update model)
  // --------------------------------------------------
  std::swap(vbo_position, vbo_draw);
}

/************************************ tex & fbo *************************************/
/**
 * @brief intialize 2D textures for back face, front face and histogram
 */
GLuint ParticleSystem::initTex2D(int width, int height) {

  GLuint Tex2D;
  glGenTextures(1, &Tex2D);
  glBindTexture(GL_TEXTURE_2D, Tex2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT,
               NULL);
  return Tex2D;
}

/**
 * @brief enable render buffer draw the volume
 */
void ParticleSystem::enable_renderbuffers() {
  glBindFramebuffer(GL_FRAMEBUFFER, fboID);
  glBindRenderbuffer(GL_RENDERBUFFER, dboID);
}

void ParticleSystem::disable_renderbuffers() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
