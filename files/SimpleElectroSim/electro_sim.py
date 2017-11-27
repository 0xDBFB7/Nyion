#imports
from shapely.geometry import Point

import math
from decimal import *
from time import sleep
import copy
try:
    from OpenGL.GL import *
    from OpenGL.GLU import *
    from OpenGL.GLUT import *
except:
    print "OpenGL wrapper for python not found"


#The grid is m^2. Force is in newtons. Charge is Coloumbs.

data_file = open("data.csv","w+")

#display values
timestep = 0.000001
pixel_size = 20
pixel_spacing = 20
grid_width = 1000
rotation_x_speed = 0
rotation_y_speed = 10
rotation_y = 0
rotation_x = 0

#Physical constants
k_constant = 8990000000.0
electron_charge = 0.000000000000016021766
electron_mass =  0.0000000000000000000000000016726218
aluminum_mass = 0.00000000000000000000000004483263

# getcontext().prec = 50
# num = Decimal(100000000000000000000000000000000000000000000000.22222222222222222222222222226415132151515156165165)

time = 0
particles = []
for i in [(math.cos(2*math.pi/50*x)*10,math.sin(2*math.pi/50*x)*10) for x in xrange(0,50+1)]:
    particles.append({"charge": 1.0*electron_charge, "mass": electron_mass, "position": [i[0],0,i[1]], "velocity": [0,500000,0]})
#
# particles = [{"charge": 1.0*electron_charge, "mass": electron_mass, "position": [10,1,0], "velocity": [0,0,0]},
#             {"charge": 1.0*electron_charge, "mass": electron_mass, "position": [15,1,0], "velocity": [0,0,0]},
#             {"charge": 1.0*electron_charge, "mass": electron_mass, "position": [50.3,53,0.3], "velocity": [0,0,0]},
#             {"charge": 1.0*electron_charge, "mass": electron_mass, "position": [50.4,54,0.4], "velocity": [-100,0,0]}]

glutInit();
glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
glEnable(GL_MULTISAMPLE);
glutInitWindowSize(1920, 1080);
glutCreateWindow("Solver");
glViewport(0, 0, 1920, 1080);
glClearColor(1.0, 1.0, 1.0, 1.0);
glClearDepth(1.0);
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LEQUAL);
glShadeModel(GL_SMOOTH);
glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);


glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
gluPerspective(60, (1920.0/1080.0), 1, 3000);
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();


glTranslatef(0,0,0);
glRotatef(0, 45, 0.1, 0);
glRotatef(0, 0, 45, 0);
glTranslatef(-grid_width/2,-grid_width/2, -3000);

def vector(a):
    if(a > 0):
        return 1.0
    else:
        return -1.0

def draw_square(x,y,z,color):
    glPushMatrix();
    glTranslatef(x,y,z);
    glColor3f(color[0],color[1],color[2]);
    glutSolidCube(pixel_size);
    glPopMatrix();

def draw_sphere(x,y,z,color):
    glPushMatrix();
    glTranslatef(x,y,z);
    glColor3f(color[0],color[1],color[2]);
    glutSolidSphere(pixel_size/2.0,10,10);
    glPopMatrix();

glutSwapBuffers();
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glutSwapBuffers();
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


while True:
    # glTranslatef(0,0,2000);
    # glRotatef(rotation_y, 0, 0.1, 0);
    # glRotatef(45, 0.1, 0, 0);
    # glTranslatef(-grid_width/2,-grid_width/2, -2000);
    # glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    rotation_y = rotation_y_speed*time
    print("Time: {}".format(time))

    frozen_state = copy.deepcopy(particles)
    for p1_index,particle in enumerate(frozen_state):
        force_x = 0
        force_y = 0
        force_z = 0

        for second_particle in [x for i,x in enumerate(frozen_state) if i!=p1_index]:
            magnitude = (k_constant*particle['charge']*second_particle['charge']);
            distance_x = (particle["position"][0]-second_particle["position"][0])
            distance_y = (particle["position"][1]-second_particle["position"][1])
            distance_z = (particle["position"][2]-second_particle["position"][2])
            distance = math.sqrt(distance_x**2.0+distance_y**2.0+distance_z**2.0)
            e_force = (magnitude/((distance)**2.0));

            force_x += e_force*(distance_x/distance)
            force_y += e_force*(distance_y/distance)
            force_z += e_force*(distance_z/distance)
            print("Distance: {}".format(math.sqrt(distance_x**2.0+distance_y**2.0+distance_z**2.0)))

            # try:
            #     force_x += vector(distance_x)*(magnitude/((distance_x)**2.0)); #if the distance is zero...you get the point.
            # except:
            #     force_x += 0
            #
            # try:
            #     force_y += vector(distance_y)*(magnitude/((distance_y)**2.0));
            # except:
            #     force_y += 0
            #
            # try:
            #     force_z += vector(distance_z)*(magnitude/((distance_z)**2.0));
            # except:
            #     force_z += 0



        accel_x = force_x/particle['mass']
        accel_y = force_y/particle['mass']
        accel_z = force_z/particle['mass']

        particles[p1_index]["velocity"][0] += accel_x*timestep;
        particles[p1_index]["velocity"][1] += accel_y*timestep;
        particles[p1_index]["velocity"][2] += accel_z*timestep;

        particles[p1_index]["position"][0] += particles[p1_index]["velocity"][0]*timestep
        particles[p1_index]["position"][1] += particles[p1_index]["velocity"][1]*timestep
        particles[p1_index]["position"][2] += particles[p1_index]["velocity"][2]*timestep
        print("-"*20)
        print("Particle: {} X: {} Y: {} Z: {}".format(p1_index,frozen_state[p1_index]["position"][0],frozen_state[p1_index]["position"][1],frozen_state[p1_index]["position"][2]))
        print("Velocity: X: {} Y: {} Z: {}".format(particles[p1_index]["velocity"][0],particles[p1_index]["velocity"][1],particles[p1_index]["velocity"][2]))
        print("Acceleration: X: {} Y: {} Z: {}".format(accel_x,accel_y,accel_z))
        print("Force: X: {} Y: {} Z: {}".format(force_x,force_y,force_z))
        # data_file.write("{},{},{},{},{},{},{},{},{},{},\n".format(particles[p1_index]["position"][0],particles[p1_index]["position"][1],particles[p1_index]["position"][2]))
        print("-"*20)
        draw_sphere(particles[p1_index]["position"][0]*pixel_spacing,particles[p1_index]["position"][1]*pixel_spacing,particles[p1_index]["position"][2]*pixel_spacing,[0,0,0])

    print("Frame complete")
    print("#"*20)
    glutSwapBuffers();
    time += timestep
    sleep(0.01)
