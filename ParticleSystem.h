
#include "RenderPlugin.h"
#include "glm.hpp"
#include "GL/gl3w.h"
#include "FramebufferObject.h"
#include "GLShader.h"
#include "VertexArray.h"

class OGL4COREPLUGIN_API ParticleSystem : public RenderPlugin {
public:
  ParticleSystem(COGL4CoreAPI *Api);
  ~ParticleSystem(void);

  virtual bool Activate(void);
  virtual bool Deactivate(void);
  virtual bool Init(void);
  virtual bool Keyboard(unsigned char key, int x, int y);
  virtual bool Motion(int x, int y);
  virtual bool Mouse(int button, int state, int x, int y);
  virtual bool Render(void);
  virtual bool Resize(int w, int h);

protected:
  void OnColorChanged(APIVar<ParticleSystem, Color3FVarPolicy> &var);
  void OnnumPointsChanged(APIVar<ParticleSystem, IntVarPolicy> &var);
  void onObjectChanged(EnumVar<ParticleSystem> &var);

private:

  //APIVars
  APIVar<ParticleSystem, FloatVarPolicy> fovY;
  APIVar<ParticleSystem, FloatVarPolicy> simulationSpeed;
  APIVar<ParticleSystem, IntVarPolicy> num_points;
  APIVar<ParticleSystem, IntVarPolicy> num_instances;
  APIVar<ParticleSystem, FloatVarPolicy> scale;
  APIVar<ParticleSystem, FloatVarPolicy> mouse_Z;
  APIVar<ParticleSystem, FloatVarPolicy> velocityCoeff;
  APIVar<ParticleSystem, FloatVarPolicy> pointSize;
  APIVar<ParticleSystem, BoolVarPolicy>  stopMotion;
  APIVar<ParticleSystem, BoolVarPolicy>  showBox;
  APIVar<ParticleSystem, Color3FVarPolicy> velocity;
  EnumVar<ParticleSystem> importObject;

  //Window
  int wWidth, wHeight; //!< window width and height
  float viewAspect;    //!< view aspect ratio

  //Shaders & VAs
  GLShader shaderQuad;  //!< shader program for Quad rendering
  std::string vsQuad;   //!< vertex shader filename for Quad rendering
  std::string fsQuad;   //!< fragment shader filename for Quad rendering
  VertexArray vaQuad;   //!< vertex array for Quad

  GLShader shaderBox;   //!< shader program for Box rendering
  std::string vsBox;    //!< vertex shader filename for Box rendering
  std::string fsBox;    //!< fragment shader filename for Box rendering
  VertexArray vaBox;    //!< vertex array for Box

  GLShader shaderCube;  //!< shader program for Cube rendering
  std::string vsCube;   //!< vertex shader filename for Cube rendering
  std::string gsCube;   //!< geometry shader filename for Cube rendering
  std::string fsCube;   //!< fragment shader filename for Cube rendering
  VertexArray vaCube;   //!< vertex array for Cube

  GLShader shaderCtrlPoints;    //!< shader program for CtrlPoints rendering
  std::string vsCtrlPoints;     //!< vertex shader filename for CtrlPoints rendering
  std::string gsCtrlPoints;     //!< geometry shader filename for CtrlPoints renderin
  std::string fsCtrlPoints;     //!< fragment shader filename for CtrlPoints renderin
  VertexArray vaCtrlPoints;     //!< vertex array for CtrlPoints

  //transform feedback
  GLuint va_points;      //!< vertex array for CtrlPoints
  GLuint vbo_position;  //!< vertex buffer object for CtrlPoints position
  GLuint vbo_speed;     //!< vertex buffer object for CtrlPoints speed
  GLuint vbo_ID;        //!< vertex buffer object for CtrlPoints ID

  //feedback buffers
  GLuint va_draw;
  GLuint vbo_draw;
  GLuint vbo_draw_speed;

  void createTransformProgram(std::string pathName);
  GLuint program_transform;
  GLuint FeedbackID;
  GLShader shaderTransform;
  GLShader shaderObject;
  std::string vsObject;
  std::string gsObject;
  std::string fsObject;

  GLuint va_sphere;
  GLuint vbo_sphere;
  GLuint ibo_sphere;

  GLuint vbo_torus;
  GLuint ibo_torus;
  GLuint va_torus;

  //object textures
  GLuint tex_cube;
  GLuint tex_sphere;
  GLuint tex_torus;
  GLuint tex_ef;
  GLuint tex_ef5;

  //Matrices
  glm::mat4 modelMX; //!< model matrix
  glm::mat4 viewMX;  //!< view matrix
  glm::mat4 projMX;  //!< projection matrix

  //create objects
  void createCtrlPoints();
  void createBox();
  void createCube();
  void createSphere();
  void createTorus();
  void createEF(std::string pathName, bool first);

  // compile
  bool checkProgram(std::string pathName, std::string vertexPath,
                    std::string fragPath);
  bool checkProgram(std::string pathName, std::string vertexPath,
                    std::string fragPath, std::string geomPath);
  bool checkShaderStatus(GLuint shaderID);
  bool checkPogramStatus(GLuint programID);
  std::string readShaderCode(const char *fileName);
  void checkFramebufferStatus();

  // tex&fbo
  int offset_velocity = 0;
  int random_tex_size = 100;
  float texdata[40000];
  GLuint fboID, dboID;
  GLuint texBoard;
  GLuint randomTex;
  GLuint colAttachID[1]; //!< handles for color attachments
  bool initFBO();
  GLuint initTex2D(int width, int height);
  void enable_renderbuffers();
  void disable_renderbuffers();

  // Draw
  void drawToFBO();
  void drawQuad();
  void drawBox();
  void drawObject();
  void drawPoints();
  void updateParticles();
  void setUniforms();
  int objectIndex = 0;
  int NUM_EF_VERTICES= 0;

  //motion
  const float deltaT = 0.0001f;
  float currentTime = 0.0f;
  glm::vec3 targetPoint;
  glm::vec3 targetPoint_old;
  glm::vec3 direction;
  int counter = 0;
  int tex_counter = 0;
  float coeff = 1.0f;
  glm::vec3 targetSpeed;
  glm::vec3 targetSpeed_copy;
};

extern "C" OGL4COREPLUGIN_API RenderPlugin *OGL4COREPLUGIN_CALL
CreateInstance(COGL4CoreAPI *Api) {
  return new ParticleSystem(Api);
}
