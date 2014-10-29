#include "Scene.h"
#include <GL/glu.h>
#include <math.h>

///////////////////////////////////////////////////////
// Useful constants
#define GLT_PI_DIV_180 0.017453292519943296

///////////////////////////////////////////////////////////////////////////////
// Useful shortcuts and macros
// Radians are king... but we need a way to swap back and forth
#define gltDegToRad(x)	((x)*GLT_PI_DIV_180)

Scene::Scene( QWidget *parent ) :
    QGLWidget( parent )
{
    this->setFocusPolicy( Qt::StrongFocus );
}

void Scene::initializeGL()
{
    // Bluish background
    glClearColor(0.0f, 0.0f, .50f, 1.0f );

    // Draw everything as wire frame
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    gltInitFrame( &frameCamera );  // Initialize the camera
}

void Scene::paintGL()
{
    // Clear the window with current clearing color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    gltApplyCameraTransform(&frameCamera);

    // Draw the ground
    drawGround();

    glPopMatrix();
}

void Scene::resizeGL(int w, int h)
{
    GLfloat fAspect;

    // Prevent a divide by zero, when window is too short
    // (you cant make a window of zero width).
    if(h == 0)
        h = 1;

    glViewport(0, 0, w, h);

    fAspect = (GLfloat)w / (GLfloat)h;

    // Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Set the clipping volume
    gluPerspective(35.0f, fAspect, 1.0f, 50.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Scene::keyPressEvent( QKeyEvent *event )
{
    switch ( event->key() ) {
        case Qt::Key_Up:
            gltMoveFrameForward(&frameCamera, 0.1f);
            break;
        case Qt::Key_Down:
            gltMoveFrameForward(&frameCamera, -0.1f);
            break;
        case Qt::Key_Left:
            gltRotateFrameLocalY(&frameCamera, 0.1);
            break;
        case Qt::Key_Right:
            gltRotateFrameLocalY(&frameCamera, -0.1);
            break;
    }

    updateGL();
}

///////////////////////////////////////////////////////////
// Draw a gridded ground
void Scene::drawGround()
{
    GLfloat fExtent = 20.0f;
    GLfloat fStep = 1.0f;
    GLfloat y = -0.4f;
    GLint iLine;

    glBegin(GL_LINES);
    for( iLine = -fExtent; iLine <= fExtent; iLine += fStep ) {
        glVertex3f(iLine, y, fExtent);    // Draw Z lines
        glVertex3f(iLine, y, -fExtent);

        glVertex3f(fExtent, y, iLine);
        glVertex3f(-fExtent, y, iLine);
    }

    glEnd();
}

//////////////////////////////////////////////////////////////////
// Apply a camera transform given a frame of reference. This is
// pretty much just an alternate implementation of gluLookAt using
// floats instead of doubles and having the forward vector specified
// instead of a point out in front of me.
void Scene::gltApplyCameraTransform(GLTFrame *pCamera)
{
    GLTMatrix mMatrix;
    GLTVector3 vAxisX;
    GLTVector3 zFlipped;

    zFlipped[0] = -pCamera->vForward[0];
    zFlipped[1] = -pCamera->vForward[1];
    zFlipped[2] = -pCamera->vForward[2];

    // Derive X vector
    gltVectorCrossProduct(pCamera->vUp, zFlipped, vAxisX);

    // Populate matrix, note this is just the rotation and is transposed
    mMatrix[0] = vAxisX[0];
    mMatrix[4] = vAxisX[1];
    mMatrix[8] = vAxisX[2];
    mMatrix[12] = 0.0f;

    mMatrix[1] = pCamera->vUp[0];
    mMatrix[5] = pCamera->vUp[1];
    mMatrix[9] = pCamera->vUp[2];
    mMatrix[13] = 0.0f;

    mMatrix[2] = zFlipped[0];
    mMatrix[6] = zFlipped[1];
    mMatrix[10] = zFlipped[2];
    mMatrix[14] = 0.0f;

    mMatrix[3] = 0.0f;
    mMatrix[7] = 0.0f;
    mMatrix[11] = 0.0f;
    mMatrix[15] = 1.0f;

    // Do the rotation first
    glMultMatrixf(mMatrix);

    // Now, translate backwards
    glTranslatef( -pCamera->vLocation[0],
            -pCamera->vLocation[1],
            -pCamera->vLocation[2]);
}

// Calculate the cross product of two vectors
void Scene::gltVectorCrossProduct(const GLTVector3 vU,
                                  const GLTVector3 vV,
                                  GLTVector3 vResult)
{
    vResult[0] = vU[1]*vV[2] - vV[1]*vU[2];
    vResult[1] = -vU[0]*vV[2] + vV[0]*vU[2];
    vResult[2] = vU[0]*vV[1] - vV[0]*vU[1];
}

// Initialize a frame of reference.
// Uses default OpenGL viewing position and orientation
void Scene::gltInitFrame(GLTFrame *pFrame)
{
    pFrame->vLocation[0] = 0.0f;
    pFrame->vLocation[1] = 0.0f;
    pFrame->vLocation[2] = 0.0f;

    pFrame->vUp[0] = 0.0f;
    pFrame->vUp[1] = 1.0f;
    pFrame->vUp[2] = 0.0f;

    pFrame->vForward[0] = 0.0f;
    pFrame->vForward[1] = 0.0f;
    pFrame->vForward[2] = -1.0f;
}

/////////////////////////////////////////////////////////
// March a frame of reference forward. This simply moves
// the location forward along the forward vector.
void Scene::gltMoveFrameForward(GLTFrame *pFrame, GLfloat fStep)
{
    pFrame->vLocation[0] += pFrame->vForward[0] * fStep;
    pFrame->vLocation[1] += pFrame->vForward[1] * fStep;
    pFrame->vLocation[2] += pFrame->vForward[2] * fStep;
}

/////////////////////////////////////////////////////////
// Rotate a frame around it's local Y axis
void Scene::gltRotateFrameLocalY(GLTFrame *pFrame, GLfloat fAngle)
{
    GLTMatrix mRotation;
    GLTVector3 vNewForward;

    gltRotationMatrix((float)gltDegToRad(fAngle), 0.0f, 1.0f, 0.0f, mRotation);
    gltRotationMatrix(fAngle, pFrame->vUp[0], pFrame->vUp[1], pFrame->vUp[2], mRotation);

    gltRotateVector(pFrame->vForward, mRotation, vNewForward);
    memcpy(pFrame->vForward, vNewForward, sizeof(GLTVector3));
}

///////////////////////////////////////////////////////////////////////////////
// Creates a 4x4 rotation matrix, takes radians NOT degrees
void Scene::gltRotationMatrix(float angle, float x, float y, float z,
                              GLTMatrix mMatrix)
{
    float vecLength, sinSave, cosSave, oneMinusCos;
    float xx, yy, zz, xy, yz, zx, xs, ys, zs;

    // If NULL vector passed in, this will blow up...
    if(x == 0.0f && y == 0.0f && z == 0.0f)
        {
        gltLoadIdentityMatrix(mMatrix);
        return;
        }

    // Scale vector
    vecLength = (float)sqrt( x*x + y*y + z*z );

    // Rotation matrix is normalized
    x /= vecLength;
    y /= vecLength;
    z /= vecLength;

    sinSave = (float)sin(angle);
    cosSave = (float)cos(angle);
    oneMinusCos = 1.0f - cosSave;

    xx = x * x;
    yy = y * y;
    zz = z * z;
    xy = x * y;
    yz = y * z;
    zx = z * x;
    xs = x * sinSave;
    ys = y * sinSave;
    zs = z * sinSave;

    mMatrix[0] = (oneMinusCos * xx) + cosSave;
    mMatrix[4] = (oneMinusCos * xy) - zs;
    mMatrix[8] = (oneMinusCos * zx) + ys;
    mMatrix[12] = 0.0f;

    mMatrix[1] = (oneMinusCos * xy) + zs;
    mMatrix[5] = (oneMinusCos * yy) + cosSave;
    mMatrix[9] = (oneMinusCos * yz) - xs;
    mMatrix[13] = 0.0f;

    mMatrix[2] = (oneMinusCos * zx) - ys;
    mMatrix[6] = (oneMinusCos * yz) + xs;
    mMatrix[10] = (oneMinusCos * zz) + cosSave;
    mMatrix[14] = 0.0f;

    mMatrix[3] = 0.0f;
    mMatrix[7] = 0.0f;
    mMatrix[11] = 0.0f;
    mMatrix[15] = 1.0f;
}

///////////////////////////////////////////////////////////////////////////////
// Load a matrix with the Idenity matrix
void Scene::gltLoadIdentityMatrix(GLTMatrix m)
{
    static GLTMatrix identity = { 1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 1.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 1.0f };

    memcpy(m, identity, sizeof(GLTMatrix));
}

// Rotates a vector using a 4x4 matrix. Translation column is ignored
void Scene::gltRotateVector( const GLTVector3 vSrcVector,
                             const GLTMatrix mMatrix,
                             GLTVector3 vOut)
{
    vOut[0] = mMatrix[0] * vSrcVector[0] + mMatrix[4] * vSrcVector[1] + mMatrix[8] *  vSrcVector[2];
    vOut[1] = mMatrix[1] * vSrcVector[0] + mMatrix[5] * vSrcVector[1] + mMatrix[9] *  vSrcVector[2];
    vOut[2] = mMatrix[2] * vSrcVector[0] + mMatrix[6] * vSrcVector[1] + mMatrix[10] * vSrcVector[2];
}
