//  Train Project -
// Train Window class implementation
// - note: this is code that a student might want to modify for their project
//   see the header file for more details
// - look for the TODO: in this file
// - also, look at the "TrainView" - its the actual OpenGL window that
//   we draw into
//
// Written by Mike Gleicher, October 2008
//

//////////////////////////////////////////////////////////////////////////
//Please fill your name and student ID
//Name:
//StuID:
//////////////////////////////////////////////////////////////////////////


#include "TrainWindow.H"
#include "TrainView.H"
#include "CallBacks.H"

#include <FL/fl.h>
#include <FL/Fl_Box.h>

// for using the real time clock
#include <time.h>
#include <math.h>

/////////////////////////////////////////////////////
void tensionCallback(Fl_Widget*, TrainWindow* tw);
void noOfCarsCallback(Fl_Widget*, TrainWindow* tw);

TrainWindow::TrainWindow(const int x, const int y) : Fl_Double_Window(x,y,800,600,"Train and Roller Coaster")
{
	// make all of the widgets
	begin();	// add to this widget
	{
		int pty=5;			// where the last widgets were drawn

		trainView = new TrainView(5,5,590,590);
		trainView->tw = this;
		trainView->world = &world;
		this->resizable(trainView);

		// to make resizing work better, put all the widgets in a group
		widgets = new Fl_Group(600,5,190,590);
		widgets->begin();

		runButton = new Fl_Button(605,pty,60,20,"Run");
		togglify(runButton, 1);

		Fl_Button* fb = new Fl_Button(700,pty,25,20,"@>>");
		fb->callback((Fl_Callback*)forwCB,this);
		Fl_Button* rb = new Fl_Button(670,pty,25,20,"@<<");
		rb->callback((Fl_Callback*)backCB,this);
		
		arcLength = new Fl_Button(730,pty,65,20,"ArcLength");
		togglify(arcLength,1);
  
		pty+=25;
		speed = new Fl_Value_Slider(655,pty,140,20,"speed");
		speed->range(0,2);
		speed->step(0.01);
		speed->value(0.5);
		speed->align(FL_ALIGN_LEFT);
		speed->type(FL_HORIZONTAL);

		pty += 30;

		// camera buttons - in a radio button group
		Fl_Group* camGroup = new Fl_Group(600,pty,195,20);
		camGroup->begin();
		worldCam = new Fl_Button(605, pty, 60, 20, "World");
        worldCam->type(FL_RADIO_BUTTON);		// radio button
        worldCam->value(1);			// turned on
        worldCam->selection_color((Fl_Color)3); // yellow when pressed
		worldCam->callback((Fl_Callback*)damageCB,this);
		trainCam = new Fl_Button(670, pty, 60, 20, "Train");
        trainCam->type(FL_RADIO_BUTTON);
        trainCam->value(0);
        trainCam->selection_color((Fl_Color)3);
		trainCam->callback((Fl_Callback*)damageCB,this);
		topCam = new Fl_Button(735, pty, 60, 20, "Top");
        topCam->type(FL_RADIO_BUTTON);
        topCam->value(0);
        topCam->selection_color((Fl_Color)3);
		topCam->callback((Fl_Callback*)damageCB,this);
		camGroup->end();

		pty += 30;

		// browser to select spline types
		// TODO: make sure these choices are the same as what the code supports
		splineBrowser = new Fl_Browser(605,pty,120,75,"Spline Type");
		splineBrowser->type(2);		// select
		splineBrowser->callback((Fl_Callback*)damageCB,this);
		splineBrowser->add("Linear");
		splineBrowser->add("Cardinal Cubic");
		splineBrowser->select(2);

		pty += 110;

		// add and delete points
		Fl_Button* ap = new Fl_Button(605,pty,80,20,"Add Point");
		ap->callback((Fl_Callback*)addPointCB,this);
		Fl_Button* dp = new Fl_Button(690,pty,80,20,"Delete Point");
		dp->callback((Fl_Callback*)deletePointCB,this);

		pty += 25;
		// reset the points
		resetButton = new Fl_Button(735,pty,60,20,"Reset");
		resetButton->callback((Fl_Callback*)resetCB,this);
		Fl_Button* loadb = new Fl_Button(605,pty,60,20,"Load");
		loadb->callback((Fl_Callback*) loadCB, this);
		Fl_Button* saveb = new Fl_Button(670,pty,60,20,"Save");
		saveb->callback((Fl_Callback*) saveCB, this);

		pty += 25;
		// roll the points
		Fl_Button* rx = new Fl_Button(605,pty,30,20,"R+X");
		rx->callback((Fl_Callback*)rpxCB,this);
		Fl_Button* rxp = new Fl_Button(635,pty,30,20,"R-X");
		rxp->callback((Fl_Callback*)rmxCB,this);
		Fl_Button* rz = new Fl_Button(670,pty,30,20,"R+Z");
		rz->callback((Fl_Callback*)rpzCB,this);
		Fl_Button* rzp = new Fl_Button(700,pty,30,20,"R-Z");
		rzp->callback((Fl_Callback*)rmzCB,this);

		pty+=30;

		// TODO: add widgets for all of your fancier features here
		Fl_Value_Slider* tension = new Fl_Value_Slider(655,pty,140,20,"Tension");
		tension->range(0, 1);
		tension->value(1.0);
		tension->step(0.05);
		tension->align(FL_ALIGN_LEFT);
		tension->type(FL_HORIZONTAL);
		tension->callback((Fl_Callback*)tensionCallback,this);

		pty+=20;

		Fl_Value_Slider* noOfCars = new Fl_Value_Slider(655,pty,140,20,"Cars");
		noOfCars->range(1, 10);
		noOfCars->value(3);
		noOfCars->step(1);
		noOfCars->align(FL_ALIGN_LEFT);
		noOfCars->type(FL_HORIZONTAL);
		noOfCars->callback((Fl_Callback*)noOfCarsCallback,this);

		// we need to make a little phantom widget to have things resize correctly
		Fl_Box* resizebox = new Fl_Box(600,595,200,5);
		widgets->resizable(resizebox);

		widgets->end();
	}
	end();	// done adding to this widget

	// set up callback on idle
	Fl::add_idle((void (*)(void*))runButtonCB,this);
}

// handy utility to make a button into a toggle
void TrainWindow::togglify(Fl_Button* b, int val)
{
    b->type(FL_TOGGLE_BUTTON);		// toggle
    b->value(val);		// turned off
    b->selection_color((Fl_Color)3); // yellow when pressed	
	b->callback((Fl_Callback*)damageCB,this);
}

void TrainWindow::damageMe()
{
	if (trainView->selectedCube >= ((int)world.points.size()))
		trainView->selectedCube = 0;
	trainView->damage(1);
}

void tensionCallback(Fl_Widget* tension_widget, TrainWindow* tw)
{
	tw->world.tension = (float)((Fl_Value_Slider*)tension_widget)->value();
	tw->damageMe();
}

void noOfCarsCallback(Fl_Widget* noOfCars_widget, TrainWindow* tw)
{
	tw->world.no_of_cars = (float)((Fl_Value_Slider*)noOfCars_widget)->value();
	tw->damageMe();
}

Pnt3f getLocationFromParameter(World *world, float para, float tension)
{
	int start_point_index = (int)para + world->points.size();
	float t = para - (int)para;
	Pnt3f location;
	Pnt3f control_points[4];

	location.x = 0.0f;
	location.y = 0.0f;
	location.z = 0.0f;
	control_points[0] = world->points[(start_point_index - 1) % world->points.size()].pos;
	control_points[1] = world->points[(start_point_index + 0) % world->points.size()].pos;
	control_points[2] = world->points[(start_point_index + 1) % world->points.size()].pos;
	control_points[3] = world->points[(start_point_index + 2) % world->points.size()].pos;

	const float t2 = t*t;
	const float t3 = t*t*t;
	location = location + tension * (-t3 + 2*t2 - t) * control_points[0];
	location = location + ((2*t3 - 3*t2 + 1) + tension * (t2 - t3)) * control_points[1];
	location = location + ((-2*t3 + 3*t2) + tension * (t3 - 2*t2 + t)) * control_points[2];
	location = location + tension * (t3 - t2) * control_points[3];

	return location;
}

float distance(Pnt3f a, Pnt3f b)
{
	return sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z));
}

void getNextPoint(World *world, float displacement, float tension, float &para, Pnt3f &next_loc)
{
	const float du = 0.01f;
	float current_displacement = 0.0f;
	Pnt3f current_loc = getLocationFromParameter(world, para, tension);
	
	while (abs(current_displacement) < abs(displacement)) {
		if (displacement > 0) {
			para += du;
		} else {
			para -= du;
			if (para < 0.0001) {
				para += (float)world->points.size();
			}
		}

		next_loc = getLocationFromParameter(world, para, tension);

		if (displacement > 0) {
			current_displacement += distance(current_loc, next_loc);
		} else {
			current_displacement -= distance(current_loc, next_loc);
		}
		
		current_loc = next_loc;
	}
}

/////////////////////////////////////////////////////
// this will get called (approximately) 30 times per second
// if the run button is pressed
void TrainWindow::advanceTrain(float dir)
{
	// TODO: make this work for your train

	// note - we give a little bit more example code here than normal,
	// so you can see how this works
	bool isUsePhysic = true;
	const float gravity = 1.0f;

	if (arcLength->value()) {
		//considering the arc length requirement
		float trainSpeed = 0.0f;
		float minSpeed = 0.001f;

		if (isUsePhysic) {
			if (world.points.size() > 0) {
				float lowestHeight = world.points[0].pos.y;
				float highestHeight = world.points[0].pos.y;

				for (int i = 1; i < world.points.size(); i++) {
					if (world.points[i].pos.y < lowestHeight) {
						lowestHeight = world.points[i].pos.y;
					}

					if (world.points[i].pos.y > highestHeight) {
						highestHeight = world.points[i].pos.y;
					}
				}

				const float totalEnergy = gravity * (highestHeight) + 0.2;
				trainSpeed = std::max(minSpeed, sqrt(2*(totalEnergy - gravity * (world.train_height))));
			}
		} else {
			trainSpeed = 10;
		}

		Pnt3f next_loc;
		trainSpeed = trainSpeed * (float)speed->value() * dir;
		getNextPoint(&world, trainSpeed, world.tension, world.trainU, next_loc);
	} else {
		//the basic functionality
		world.trainU +=  dir * ((float)speed->value() * .1f);
	}

	float nct = static_cast<float>(world.points.size());
	if (world.trainU > nct) world.trainU -= nct;
	if (world.trainU < 0) world.trainU += nct;

}