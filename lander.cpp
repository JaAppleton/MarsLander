// Mars lander simulator
// Version 1.11
// Mechanical simulation functions
// Gabor Csanyi and Andrew Gee, August 2019

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation, to make use of it
// for non-commercial purposes, provided that (a) its original authorship
// is acknowledged and (b) no modified versions of the source code are
// published. Restriction (b) is designed to protect the integrity of the
// exercise for future generations of students. The authors would be happy
// to receive any suggested modifications by private correspondence to
// ahg@eng.cam.ac.uk and gc121@eng.cam.ac.uk.

#include "lander.h"
using namespace std;

void autopilot (void)
  // Autopilot to adjust the engine throttle, parachute and attitude control
{
    double Kh = 0.03;
    double Kp = 0.5;
    double delta = 0.5;

    double h = position.abs() - MARS_RADIUS;
    double rad_desc_rate = (position*velocity)/position.abs();
    double e = -(0.5 + Kh * h + rad_desc_rate);
    double Pout = Kp * e;

    if (Pout <= -delta) {
        throttle = 0;
    }
    else if (Pout >= 1 - delta) {
        throttle = 1;
    }
    else {
        throttle = delta + Pout;
    }


}


void numerical_dynamics (void)
  // This is the function that performs the numerical integration to update the
  // lander's pose. The time step is delta_t (global variable).
{
  //Previous pos for Verlet
    static vector3d position_prev = position;
    vector3d new_position;
    

    //Mass of lander
    double mass = UNLOADED_LANDER_MASS + (fuel* FUEL_CAPACITY * FUEL_DENSITY);

    //Forces
    vector3d gravity = -(GRAVITY * MARS_MASS * mass / (position.abs2())) * position.norm();
    vector3d thrust = thrust_wrt_world();
    vector3d drag;

    //Drag forces from chute 
    if (parachute_status == DEPLOYED) {
        double drag_lander = 0.5 * atmospheric_density(position) * DRAG_COEF_LANDER * ( LANDER_SIZE * PI * LANDER_SIZE) * velocity.abs2();
        double drag_chute = 0.5 * atmospheric_density(position) * DRAG_COEF_CHUTE * (5 * 2 * LANDER_SIZE * 2 * LANDER_SIZE) * velocity.abs2();
        drag = -velocity.norm() * (drag_lander + drag_chute);
    }
    else {
        double drag_lander = 0.5 * atmospheric_density(position) * DRAG_COEF_LANDER * (LANDER_SIZE * PI * LANDER_SIZE) * velocity.abs2();
        drag = -velocity.norm() * drag_lander;
    }
    
   
    //Net force
    vector3d force_net = gravity + thrust + drag;

    //acceleration
    vector3d acceleration = force_net / mass;

        //Choose integration method (Euler or verlet)
    bool use_verlet = true; //toggle for different integration methods

    if (use_verlet) {

        //verlet integration
        if (simulation_time == 0.0) {

            position_prev = position;
            cout << "Initial Position: " << position << endl;
            cout << "Initial Velocity: " << velocity << endl;
            cout << "Initial Acceleration: " << acceleration << endl;
           
            //Use euler for the first iteration
            new_position = position + velocity * delta_t + 0.5 * acceleration * delta_t * delta_t;
            vector3d new_velocity = velocity + acceleration * delta_t;

            //update position_prev for next it.
            position = new_position;
            velocity = new_velocity;
            cout << "Updated Position(first iteration)" << position << endl;
            

        }


        else {
            
            
            new_position = 2 * position - position_prev + acceleration * delta_t * delta_t;
            vector3d new_velocity = (new_position - position_prev) / (2 * delta_t);

            

            //update for next iteration
            position_prev = position;
            position = new_position;
            velocity = new_velocity;
        }

        
    }
    else {
        //euler integration
        velocity = velocity + acceleration * delta_t;
        position = position + velocity * delta_t;

    }

  // Here we can apply an autopilot to adjust the thrust, parachute and attitude
  if (autopilot_enabled) autopilot();

  // Here we can apply 3-axis stabilization to ensure the base is always pointing downwards
  if (stabilized_attitude) attitude_stabilization();
}

void initialize_simulation (void)
  // Lander pose initialization - selects one of 10 possible scenarios
{
  // The parameters to set are:
  // position - in Cartesian planetary coordinate system (m)
  // velocity - in Cartesian planetary coordinate system (m/s)
  // orientation - in lander coordinate system (xyz Euler angles, degrees)
  // delta_t - the simulation time step
  // boolean state variables - parachute_status, stabilized_attitude, autopilot_enabled
  // scenario_description - a descriptive string for the help screen

  scenario_description[0] = "circular orbit";
  scenario_description[1] = "descent from 10km";
  scenario_description[2] = "elliptical orbit, thrust changes orbital plane";
  scenario_description[3] = "polar launch at escape velocity (but drag prevents escape)";
  scenario_description[4] = "elliptical orbit that clips the atmosphere and decays";
  scenario_description[5] = "descent from 200km";
  scenario_description[6] = "";
  scenario_description[7] = "";
  scenario_description[8] = "";
  scenario_description[9] = "";

  switch (scenario) {

  case 0:
    // a circular equatorial orbit
    position = vector3d(1.2*MARS_RADIUS, 0.0, 0.0);
    velocity = vector3d(0.0, -3247.087385863725, 0.0);
    orientation = vector3d(0.0, 90.0, 0.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 1:
    // a descent from rest at 10km altitude
    position = vector3d(0.0, -(MARS_RADIUS + 10000.0), 0.0);
    velocity = vector3d(0.0, 0.0, 0.0);
    orientation = vector3d(0.0, 0.0, 90.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = true;
    autopilot_enabled = false;
    break;

  case 2:
    // an elliptical polar orbit
    position = vector3d(0.0, 0.0, 1.2*MARS_RADIUS);
    velocity = vector3d(3500.0, 0.0, 0.0);
    orientation = vector3d(0.0, 0.0, 90.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 3:
    // polar surface launch at escape velocity (but drag prevents escape)
    position = vector3d(0.0, 0.0, MARS_RADIUS + LANDER_SIZE/2.0);
    velocity = vector3d(0.0, 0.0, 5027.0);
    orientation = vector3d(0.0, 0.0, 0.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 4:
    // an elliptical orbit that clips the atmosphere each time round, losing energy
    position = vector3d(0.0, 0.0, MARS_RADIUS + 100000.0);
    velocity = vector3d(4000.0, 0.0, 0.0);
    orientation = vector3d(0.0, 90.0, 0.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 5:
    // a descent from rest at the edge of the exosphere
    position = vector3d(0.0, -(MARS_RADIUS + EXOSPHERE), 0.0);
    velocity = vector3d(0.0, 0.0, 0.0);
    orientation = vector3d(0.0, 0.0, 90.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = true;
    autopilot_enabled = false;
    break;

  case 6:
    break;

  case 7:
    break;

  case 8:
    break;

  case 9:
    break;

  }
}
