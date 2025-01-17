/************************************************************************
     File:        ArcBallCam.cpp
        substantially updated October 2007, an upgrade from MyArcBall

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu
     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:    
				Written October, 2001, Michael L. Gleicher
				Updated October, 2005 for  - Michael L. Gleicher
                2008 - put into the CVS Utilities module so its easier
                       to use across different projects
			    2008 - added Mousewheel
				2008 - added double right click to reset

     See the header file for information.

     CVS Info: Date checked in: $Date: 2008/10/19 01:54:28 $
               Revision number: $Revision: 1.8 $
   
     Platform:    Visio Studio.Net 2005
*************************************************************************/

#include "ArcBallCam.H"
#include <math.h>

#include <windows.h>

// the FlTk headers have lots of warnings - these are bad, but there's not
// much we can do about them
#pragma warning(push)
#pragma warning(disable:4311)		// convert void* to long
#pragma warning(disable:4312)		// convert long to void*
#include <FL/Fl_Gl_Window.h>
#include <Fl/Fl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <Fl/Fl_Double_Window.h>
#pragma warning(pop)

#include "stdio.h"


//**************************************************************************
//
//==========================================================================
ArcBallCam::ArcBallCam() : wind(0),
						 fieldOfView(40),
						 eyeX(0), eyeY(0), eyeZ(20),
						 initEyeZ(20), mode(None), 
						 panX(0), panY(0),
						 isx(0), isy(0), isz(0)
{
}

// really do the setup stuff, once you have the window
void ArcBallCam::setup(Fl_Gl_Window* _wind, float _fieldOfView, float _eyeZ,
					   float _isx, float _isy, float _isz)
{
	wind = _wind;
	fieldOfView = _fieldOfView;
	eyeZ = _eyeZ;
	initEyeZ = _eyeZ;
	eyeX = 0;
	eyeY = 0;
	isx = _isx;
	isy = _isy;
	isz = _isz;
	reset();
	spin(isx,isy,isz);
}


void ArcBallCam::setProjection(bool doClear)
{
  glMatrixMode(GL_PROJECTION);
  if (doClear)
	  glLoadIdentity();

  // Compute the aspect ratio so we don't distort things
  double aspect = ((double) wind->w()) / ((double) wind->h());
  gluPerspective(fieldOfView, aspect, .1, 1000);

  // Put the camera where we want it to be
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Use the transformation in the ArcBall
  glTranslatef(-eyeX, -eyeY, -eyeZ);
  multMatrix();
}

int ArcBallCam::handle(int e)
{
	switch(e) {
		case FL_PUSH:
			mode = None;
			if (Fl::event_button() == 3) { // right mouse button down
				if (Fl::event_clicks()) {	// double click? do a reset
					setup(wind,fieldOfView,initEyeZ,isx,isy,isz);
					wind->damage(1);
					return 1;
				}
				float x,y;
				getMouseNDC(x,y);
				down(x,y);
				wind->damage(1);
				mode = (Fl::event_state() & FL_ALT) ? Pan : Rotate;
				return 1;
			};
			break;
		case FL_RELEASE:
			if (mode != None) {
				wind->damage(1);
				mode = None;
				return 1;
			}
			break;
		case FL_DRAG: // if the user drags the mouse
			if(mode != None) { // we're taking the drags
				float x,y;
				getMouseNDC(x,y);
				computeNow(x,y);
				wind->damage(1);
				return 1;
			};
			break;
		case FL_MOUSEWHEEL: {
			float zamt = (Fl::event_dy() < 0) ? 1.1f : 1/1.1f;
			eyeZ *= zamt;
			wind->damage(1);
			return 1;
			};
			break;

	}
	return 0;
}

//**************************************************************************
//
// Get the mouse in NDC
//==========================================================================
void ArcBallCam::getMouseNDC(float& x, float& y)
//==========================================================================
{
	// notice, we put everything into doubles so we can do the math
	float mx = (float) Fl::event_x();	// remeber where the mouse went down
	float my = (float) Fl::event_y();

	// we will assume that the viewport is the same as the window size
	float wd = (float) wind->w();
	float hd = (float) wind->h();

	// remember that FlTk has Y going the wrong way!
	my = hd-my;

	x = (mx / wd) * 2.0f - 1.f;
	y = (my / hd) * 2.0f - 1.f;
}

//**************************************************************************
//
// when the mouse goes down, remember where. also, clear out the now
// position by putting it into start
//==========================================================================
void ArcBallCam::down(const float x, const float y)
//==========================================================================
{
	start = now * start;
	now = Quat();		// identity

	downX = x;
	downY = y;

	panX = 0;
	panY = 0;
}

//**************************************************************************
//
// get the whole matrix!
//==========================================================================
void ArcBallCam::getMatrix(HMatrix m) const
//==========================================================================
{
	Quat qAll = now * start;
	qAll = qAll.conjugate();   // since Ken does everything transposed
	qAll.toMatrix(m);
}

//**************************************************************************
//
// a simplified interface - so you never see the insides of arcball
//==========================================================================
void ArcBallCam::multMatrix()
//==========================================================================
{
	HMatrix m;
	getMatrix(m);
    glMultMatrixf((float*) m);
}
//**************************************************************************
//
//==========================================================================
void ArcBallCam::reset()
//==========================================================================
{
	start.x = 0;
	start.y = 0;
	start.z = 0;
	start.w = 1;

	now.x = now.y = now.z = 0; now.w = 1;
}

//**************************************************************************
//
//==========================================================================
void ArcBallCam::spin(float x, float y, float z)
//==========================================================================
{
	// printf("\nstart was %g %g %g %g\n",start.x,start.y,start.z,start.w);
	// first, get rid of anything cached
	start = now * start;
	now = Quat();

	float iw = x*x + y*y + z*z;
	if (iw<1)
		iw = sqrt(1-iw);
	else
		iw = 0;
	Quat newq(x,y,z,iw);

	newq.renorm();
	start = newq * start;
	// printf(" (no norm) %g %g %g %g\n",start.x,start.y,start.z,start.w);
	start.renorm();
	// printf("start is %g %g %g %g\n",start.x,start.y,start.z,start.w);
}

//**************************************************************************
//
// the gory guts

//**************************************************************************
//
// a helper - figure out where in XYZ a mouse down goes
// assumes sphere of unit radius
//==========================================================================
static void onUnitSphere(const float mx, const float my,
													float& x, float& y, float& z)
//==========================================================================
{
	x = mx;		// should divide radius
	y = my;
	float mag = x*x + y*y;
	if (mag > 1.0f) {
		float scale = 1.0f / ((float) sqrt(mag));
		x *= scale;
		y *= scale;
		z = 0;
	} else {
		z = (float) sqrt(1 - mag);
	}
}

//**************************************************************************
//
//==========================================================================
void ArcBallCam::computeNow(const float nowX, const float nowY)
//==========================================================================
{
	if (mode==Rotate) {
		float dx,dy,dz;
		float mx,my,mz;
		onUnitSphere(downX, downY, dx, dy, dz);
		onUnitSphere(nowX, nowY, mx, my, mz);

		// here we compute the quaternion between these two points
		now.x = dy*mz - dz*my;
		now.y = dz*mx - dx*mz;
		now.z = dx*my - dy*mx;
		now.w = dx*mx + dy*my + dz*mz;

		now.renorm();		// just in case...
	} else if (mode==Pan) {
		float dx = (nowX-downX) * eyeZ;
		float dy = (nowY-downY) * eyeZ;

		eyeX += panX - dx;
		eyeY += panY - dy;
		panX = dx;
		panY = dy;
	}
}

/////////////////////////////////////////////////////////////////////////
// Minimal Quaternion Class - if you don't know what a quaternion is, don't
// worry. if you do know what a quaternion is, you probably want a more complete
// implementation

//**************************************************************************
//
// minimal quaternion class
//==========================================================================
Quat::Quat(float ix, float iy, float iz, float iw) 
	:  x(ix), y(iy), z(iz), w(iw)
//==========================================================================
{
}
//**************************************************************************
//
//==========================================================================
Quat::Quat() 
	:  x(0), y(0), z(0), w(1)
//==========================================================================
{
}
//**************************************************************************
//
//==========================================================================
Quat::Quat(const Quat& q) 
	:  x(q.x), y(q.y), z(q.z), w(q.w)
//==========================================================================
{
}

//**************************************************************************
//
// Renormalize, in case things got messed up
//==========================================================================
void Quat::renorm()
//==========================================================================
{
    float Nq = 1.f / (float) sqrt(x*x + y*y + z*z + w*w);
	x *= Nq;
	y *= Nq;
	z *= Nq;
	w *= Nq;
}

//**************************************************************************
//
// Conversions
//==========================================================================
void Quat::toMatrix(HMatrix out) const
//==========================================================================
{
    float Nq = x*x + y*y + z*z + w*w;
    float s = (Nq > 0.f) ? (2.0f / Nq) : 0.f;
    float xs = x*s,	      ys = y*s,	  zs = z*s;
    float wx = w*xs,	  wy = w*ys,  wz = w*zs;
    float xx = x*xs,	  xy = x*ys,  xz = x*zs;
    float yy = y*ys,	  yz = y*zs,  zz = z*zs;
    out[X][X] = 1.0f - (yy + zz); out[Y][X] = xy + wz;          out[Z][X] = xz - wy;
    out[X][Y] = xy - wz;          out[Y][Y] = 1.0f - (xx + zz); out[Z][Y] = yz + wx;
    out[X][Z] = xz + wy;          out[Y][Z] = yz - wx;          out[Z][Z] = 1.0f - (xx + yy);
    out[X][W] = out[Y][W] = out[Z][W] = out[W][X] = out[W][Y] = out[W][Z] = 0.0f;
    out[W][W] = 1.0f;
}

//**************************************************************************
//
// quaternion operations
//==========================================================================
Quat Quat::conjugate() const
//==========================================================================
{
  return Quat(-x,-y,-z,w);
}

//**************************************************************************
//
//==========================================================================
Quat Quat::operator* (const Quat& qR) const
//==========================================================================
{
	Quat qq;
	qq.w = w*qR.w - x*qR.x - y*qR.y - z*qR.z;
	qq.x = w*qR.x + x*qR.w + y*qR.z - z*qR.y;
	qq.y = w*qR.y + y*qR.w + z*qR.x - x*qR.z;
	qq.z = w*qR.z + z*qR.w + x*qR.y - y*qR.x;
	return (qq);
}

// CVS Header - if you don't know what this is, don't worry about it
// $Header: /p/course/-gleicher/private/CVS/Utilities/ArcBallCam.cpp,v 1.8 2008/10/19 01:54:28 gleicher Exp $
