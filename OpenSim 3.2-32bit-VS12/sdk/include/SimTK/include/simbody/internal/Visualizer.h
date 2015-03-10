#ifndef SimTK_SIMBODY_VISUALIZER_H_
#define SimTK_SIMBODY_VISUALIZER_H_

/* -------------------------------------------------------------------------- *
 *                               Simbody(tm)                                  *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK biosimulation toolkit originating from           *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org/home/simbody.  *
 *                                                                            *
 * Portions copyright (c) 2010-12 Stanford University and the Authors.        *
 * Authors: Peter Eastman, Michael Sherman                                    *
 * Contributors:                                                              *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

/** @file 
Declares the Visualizer class used for collecting Simbody simulation results 
for display and interaction through the VisualizerGUI. **/

#include "simbody/internal/common.h"

#include <utility> // for std::pair

namespace SimTK {

class MultibodySystem;
class DecorationGenerator;

/** Provide simple visualization of and interaction with a Simbody simulation,
with real time control of the frame rate.\ There are several operating modes
available, including real time operation permitting responsive user interaction
with the simulation.

Frames are sent to the renderer at a regular interval that is selectable, with
a default rate of 30 frames/second. The various operating modes provide 
different methods of controlling which simulation frames are selected and how
they are synchronized for display.

<h3>Visualization modes</h3>

There are three operating modes for the Visualizer's display of simulation
results, selectable via setMode():

- <b>PassThrough</b>. This is the default mode. It sends through to the 
renderer \e every frame that is received from the simulation, slowing down the 
simulation if necessary so that the frames are presented at a selected frame 
rate. But note that the simulation time will not be synchronized to real time; 
because Simbody simulations generally proceed at a variable rate, the 
regularly-spaced output frames will represent different amounts of simulated 
time. If you want real time and simulation time synchronized, use the RealTime 
mode.

- <b>Sampling</b>. This mode is useful for monitoring a simulation that is
allowed to run at full speed. We send frames for display at a maximum rate 
given by the frame rate setting. After a frame is sent, all subsequent frames 
received from the simulation are ignored until the frame interval has passed; 
then the next received frame is displayed. This allows the simulation to 
proceed at the fastest rate possible but time will be irregular and not all 
frames generated by the simulation will be shown.

- <b>RealTime</b>. Synchronize frame times with the simulated time, slowing
down the simulation if it is running ahead of real time, as modifed by the
time scale; see setRealTimeScale(). Frames are sent to the renderer at the
selected frame rate. Smoothness is maintained by buffering up frames before 
sending them; interactivity is maintained by keeping the buffer length below 
human perception time (150-200ms). The presence and size of the buffer is 
selectable; see setDesiredBufferLengthInSec().

<h3>User interaction</h3>

The Simbody VisualizerGUI provides some user interaction of its own, for
example allowing the user to control the viewpoint and display options. User
inputs that it does not interpret locally are passed on to the simulation,
and can be intercepted by registering InputListeners with the Visualizer. The
Visualizer provides a class Visualizer::InputSilo which is an InputListener
that simply captures and queues all user input, with the intent that a running
simulation will occasionally stop to poll the InputSilo to process any input
that has been collected. 

<h3>Implementation notes</h3>

RealTime mode is worth some discussion. There is a simulation thread that
produces frames at a variable rate, and a draw thread that consumes frames at a
variable rate (by sending them to the renderer). We want to engineer things so 
that frames are sent to the renderer at a steady rate that is synchronized with
simulation time (possibly after scaling). When a thread is running too fast, 
that is easily handled by blocking the speeding thread for a while. The "too 
slow" case takes careful handling.

In normal operation, we expect the simulation to take varying amounts of
real time to generate fixed amounts of simulation time, because we prefer
to use variable time-step integrators that control errors by taking smaller
steps in more difficult circumstances, and large steps through the easy
parts of the simulation. For real time operation, the simulation must of
course *average* real time performance; we use a frame buffer to smooth
out variable delivery times. That is, frames go into the buffer at an
irregular rate but are pulled off at a regular rate. A longer buffer can
mask wider deviations in frame time, at the expense of interactive response.
In most circumstances people cannot perceive delays below about 200ms, so
for good response the total delay should be kept around that level.

Despite the buffering, there will be occasions when the simulation can't
keep up with real time. A common cause of that is that a user has paused
either the simulation or the renderer, such as by hitting a breakpoint while
debugging. In that case we deem the proper behavior to be that when we 
resume we should immediately resume real time behavior at a new start time, 
\e not attempt to catch up to the previous real time by running at high speed. 
As much as possible, we would like the simulation to behave just as it would 
have without the interruption, but with a long pause where interrupted. We
deal with this situation by introducing a notion of "adjusted real time"
(AdjRT). That is a clock that tracks the real time interval counter, but uses
a variable base offset that is used to match it to the expected simulation 
time. When the simulation is long delayed, we modify the AdjRT base when we
resume so that AdjRT once again matches the simulation time t. Adjustments
to the AdjRT base occur at the time we deliver frames to the renderer; at that
moment we compare the AdjRT reading to the frame's simulation time t and 
correct AdjRT for future frames.

You can also run in RealTime mode without buffering. In that case frames are
sent directly from the simulation thread to the renderer, but the above logic
still applies. Simulation frames that arrive earlier than the corresponding
AdjRT are delayed; frames that arrive later are drawn immediately but cause
AdjRT to be readjusted to resynchronize. Overall performance can be better
in unbuffered RealTime mode because the States provided by the simulation do
not have to be copied before being drawn. However, intermittent slower-than-
realtime frame times cannot be smoothed over; they will cause rendering delays.

PassThrough and Sampling modes are much simpler because no synchronization
is done to the simulation times. There is only a single thread and draw
time scheduling works in real time without adjustment. 

With the above explanation, you may be able to figure out most of what comes
out of the dumpStats() method which can be used to help diagnose performance
problems. **/
class SimTK_SIMBODY_EXPORT Visualizer {
public:
class FrameController; // defined below
class InputListener;   // defined in Visualizer_InputListener.h
class InputSilo;       //                 "
class Reporter;        // defined in Visualizer_Reporter.h


/** Construct a new %Visualizer for the indicated System, and launch the
visualizer display executable from a default or specified location. The camera's
"up" direction will initially be set to match the "up" direction hint that is 
stored with the supplied \a system; the default is that "up" is in the 
direction of the positive Y axis. The background will normally include a 
ground plane and sky, but if the \a system has been set to request a uniform 
background we'll use a plain white background instead. You can override the 
chosen defaults using %Visualizer methods setSystemUpDirection() and 
setBackgroundType(). 

Simbody is shipped with a separate executable program that provides the 
graphics display and collects user input. Normally that executable is 
installed in the "bin" subdirectory of the Simbody installation directory.
However, first we look in the same directory as the currently-running
executable and, if found, we will use that visualizer. If no visualizer
is found with the executable, we check if environment variables SIMBODY_HOME 
or SimTK_INSTALL_DIR exist, and look in their "bin" subdirectories if so.
Otherwise we'll look in defaultInstallDir/SimTK/bin where defaultInstallDir
is the ProgramFiles registry entry on Windows, or /usr/local on other platforms.
The other constructor allows specification of a search path that will be 
checked before attempting to find the installation directory.

The SimTK::Pathname class is used to process the supplied search path, which
can consist of absolute, working directory-relative, or executable 
directory-relative path names.

@see SimTK::Pathname **/
explicit Visualizer(const MultibodySystem& system);

/** Construct a new Visualizer for a given system, with a specified search
path for locating the SimbodyVisualizer executable. The search path is
checked \e after looking in the current executable directory, and \e before 
trying to locate the Simbody or SimTK installation directory. See the other
constructor's documentation for more information. **/
Visualizer(const MultibodySystem& system,
           const Array_<String>&  searchPath);

/** Copy constructor has reference counted, shallow copy semantics;
that is, the Visualizer copy is just another reference to the same
Visualizer object. **/
Visualizer(const Visualizer& src);
/** Copy assignment has reference counted, shallow copy semantics;
that is, the Visualizer copy is just another reference to the same
Visualizer object. **/
Visualizer& operator=(const Visualizer& src);
/** InputListener, FrameController, and DecorationGenerator objects are 
destroyed here when the last reference is deleted. **/
~Visualizer();

/** Ask the VisualizerGUI to shut itself down immediately. This will cause the
display window to close and the associated process to die. This method returns
immediately but it may be some time later when the VisualizerGUI acts on the
instruction; there is no way to wait for it to die. Normally the VisualizerGUI
will persist even after the death of the simulator connection unless you have
called setShutdownWhenDestructed() to make shutdown() get called automatically.
@see setShutdownWhenDestructed() **/
void shutdown();

/** Set the flag that determines whether we will automatically send a Shutdown
message to the VisualizerGUI when this %Visualizer object is destructed. 
Normally we allow the GUI to persist even after death of the simulator
connection, unless an explicit call to shutdown() is made. 
@see getShutdownWhenDestructed(), shutdown() **/
Visualizer& setShutdownWhenDestructed(bool shouldShutdown);

/** Return the current setting of the "shutdown when destructed" flag. By 
default this is false.
@see setShutdownWhenDestructed(), shutdown() **/
bool getShutdownWhenDestructed() const;

/** These are the operating modes for the Visualizer, with PassThrough the 
default mode. See the documentation for the Visualizer class for more
information about the modes. **/
enum Mode {
    /** Send through to the renderer every frame that is received from the
    simulator (default mode). **/
    PassThrough = 1,
    /** Sample the results from the simulation at fixed real time intervals
    given by the frame rate. **/
    Sampling    = 2,
    /** Synchronize real frame display times with the simulated time. **/
    RealTime    = 3
};

/** These are the types of backgrounds the VisualizerGUI currently supports.
You can choose what type to use programmatically, and users can override that
choice in the GUI. Each of these types may use additional data (such as the
background color) when the type is selected. **/
enum BackgroundType {
    /** Show a ground plane on which shadows may be cast, as well as a sky
    in the far background. **/
    GroundAndSky = 1,
    /** Display a solid background color that has been provided elsewhere. **/
    SolidColor   = 2
};

/** The VisualizerGUI may predefine some menus; if you need to refer to one
of those use its menu Id as defined here. Note that the id numbers here
are negative numbers, which are not allowed for user-defined menu ids. **/
enum PredefinedMenuIds {
    /** The id of the predefined View pull-down. **/
    ViewMenuId    = -1
};

/** @name               VisualizerGUI display options
These methods provide programmatic control over some of the VisualizerGUI's
display options. Typically these can be overridden by the user directly in
the GUI, but these are useful for setting sensible defaults. In particular,
the Ground and Sky background, which is the GUI default, is not appropriate
for some systems (molecules for example). **/
/**@{**/


/** Change the background mode currently in effect in the GUI.\ By default
we take the desired background type from the System, which will usually be
at its default value which is to show a ground plane and sky. You can override
that default choice with this method.
@param  background   The background type to use.
@return A reference to this Visualizer so that you can chain "set" calls. 
@note Molmodel's CompoundSystem requests a solid background by default, since
ground and sky is not the best way to display a molecule! **/
Visualizer& setBackgroundType(BackgroundType background);


/** Set the background color.\ This will be used when the solid background
mode is in effect but has no effect otherwise. This is a const method so you
can call it from within a FrameController, for example if you want to flash
the background color.
@param  color   The background color in r,g,b format with [0,1] range.
@return A const reference to this Visualizer so that you can chain "set" 
calls, provided subsequent ones are also const. **/
const Visualizer& setBackgroundColor(const Vec3& color) const;

/** Control whether shadows are generated when the GroundAndSky background
type is in effect.\ This has no effect if the ground plane is not being 
displayed. The default if for shadows to be displayed. This is a const method
so you can call it from within a FrameController.
@param  showShadows     Set true to have shadows generated; false for none.
@see setBackgroundType()
@return A const reference to this Visualizer so that you can chain "set" 
calls, provided subsequent ones are also const. **/
const Visualizer& setShowShadows(bool showShadows) const;

/** Control whether frame rate is shown in the Visualizer.\ This is a const
method so you can call it from within a FrameController.
@param  showFrameRate     Set true to show the frame rate; false for none.
@return A const reference to this Visualizer so that you can chain "set" 
calls, provided subsequent ones are also const. **/
const Visualizer& setShowFrameRate(bool showFrameRate) const;

/** Control whether simulation time is shown in the Visualizer.\ This is a const
method so you can call it from within a FrameController.
@param  showSimTime     Set true to show the simulation time; false for none.
@return A const reference to this Visualizer so that you can chain "set" 
calls, provided subsequent ones are also const. **/
const Visualizer& setShowSimTime(bool showSimTime) const;

/** Control whether frame number is shown in the Visualizer.\ This is a const
method so you can call it from within a FrameController.
@param  showFrameNumber     Set true to show the frame number; false for none.
@return A const reference to this Visualizer so that you can chain "set" 
calls, provided subsequent ones are also const. **/
const Visualizer& setShowFrameNumber(bool showFrameNumber) const;

/** Change the title on the main VisualizerGUI window.\ The default title
is Simbody \e version : \e exename, where \e version is the current Simbody
version number in major.minor.patch format and \e exename is the name of the 
executing simulation application's executable file (without suffix if any).
@param[in]      title   
    The new window title. The amount of room for the title varies; keep 
    it short.
@return A const reference to this Visualizer so that you can chain "set" 
calls, provided subsequent ones are also const. 
@see SimTK_version_simbody(), Pathname::getThisExecutablePath(),
Pathname::desconstructPathname() **/
const Visualizer& setWindowTitle(const String& title) const;
/**@}**/

/** @name                  Visualizer options
These methods are used for setting a variety of options for the Visualizer's
behavior, normally prior to sending it the first frame. **/
/**@{**/

/** Set the coordinate direction that should be considered the System's "up"
direction.\ When the ground and sky background is in use, this is the 
direction that serves as the ground plane normal, and is used as the initial
orientation for the camera's up direction (which is subsequently under user
or program control and can point anywhere). If you don't set this explicitly
here, the Visualizer takes the default up direction from the System, which
provides a method allowing the System's creator to specify it, with the +Y
axis being the default. 
@param[in]      upDirection 
    This must be one of the CoordinateAxis constants XAxis, YAxis, or ZAxis,
    or one of the opposite directions -XAxis, -YAxis, or -ZAxis.
@return A writable reference to this Visualizer so that you can chain "set" 
calls in the manner of chained assignments. **/
Visualizer& setSystemUpDirection(const CoordinateDirection& upDirection);
/** Get the value the Visualizer is using as the System "up" direction (
not to be confused with the camera "up" direction). **/
CoordinateDirection getSystemUpDirection() const;

/** Set the height at which the ground plane should be displayed when the
GroundAndSky background type is in effect.\ This is interpreted along the
system "up" direction that was specified in the Visualizer's System or was
overridden with the setSystemUpDirection() method. The default value is zero,
meaning that the ground plane passes through the ground origin.
@param          height   
    The position of the ground plane along the system "up" direction that 
    serves as the ground plane normal. Note that \a height is \e along the 
    up direction, meaning that if up is one of the negative coordinate axis
    directions a positive \a height will move the ground plane to a more 
    negative position.
@return A reference to this Visualizer so that you can chain "set" calls.
@see setSystemUpDirection(), setBackgroundType() **/
Visualizer& setGroundHeight(Real height);
/** Get the value the Visualizer considers to be the height of the ground
plane for this System.\ The value must be interpreted along the System's "up"
direction. @see setSystemUpDirection() **/
Real getGroundHeight() const;


/** Set the operating mode for the Visualizer. See \ref Visualizer::Mode for 
choices, and the discussion for the Visualizer class for meanings.
@param[in]  mode    The new Mode to use.
@return A reference to this Visualizer so that you can chain "set" calls. **/
void setMode(Mode mode);
/** Get the current mode being used by the Visualizer. See \ref Visualizer::Mode
for the choices, and the discussion for the Visualizer class for meanings. **/
Mode getMode() const;

/** Set the frame rate in frames/sec (of real time) that you want the 
Visualizer to attempt to achieve. This affects all modes. The default is 30 
frames per second. Set the frame rate to zero to return to the default 
behavior. 
@param[in]      framesPerSec
    The desired frame rate; specify as zero to get the default.
@return A reference to this Visualizer so that you can chain "set" calls.
**/
Visualizer& setDesiredFrameRate(Real framesPerSec);
/** Get the current value of the frame rate the Visualizer has been asked to 
attempt; this is not necessarily the rate actually achieved. A return value of 
zero means the Visualizer is using its default frame rate, which may be
dependent on the current operating mode. 
@see setDesiredFrameRate() for more information. **/
Real getDesiredFrameRate() const;

/** In RealTime mode we normally assume that one unit of simulated time should
map to one second of real time; however, in some cases the time units are not 
seconds, and in others you may want to run at some multiple or fraction of 
real time. Here you can say how much simulated time should equal one second of
real time. For example, if your simulation runs in seconds, but you want to 
run twice as fast as real time, then call setRealTimeScale(2.0), meaning that 
two simulated seconds will pass for every one real second. This call will have 
no immediate effect if you are not in RealTime mode, but the value will be 
remembered.

@param[in]      simTimePerRealSecond
The number of units of simulation time that should be displayed in one second
of real time. Zero or negative value will be interpeted as the default ratio 
of 1:1. 
@return A reference to this Visualizer so that you can chain "set" calls.
**/
Visualizer& setRealTimeScale(Real simTimePerRealSecond);
/** Return the current time scale, which will be 1 by default.
@see setRealTimeScale() for more information. **/
Real getRealTimeScale() const;

/** When running an interactive realtime simulation, you can smooth out changes
in simulation run rate by buffering frames before sending them on for 
rendering. The length of the buffer introduces an intentional response time 
lag from when a user reacts to when he can see a response from the simulator. 
Under most circumstances a lag of 150-200ms is undetectable. The default 
buffer length is the time represented by the number of whole frames 
that comes closest to 150ms; 9 frames at 60fps, 5 at 30fps, 4 at 24fps, etc. 
To avoid frequent block/unblocking of the simulation thread, the buffer is
not kept completely full; you can use dumpStats() if you want to see how the
buffer was used during a simulation. Shorten the buffer to improve 
responsiveness at the possible expense of smoothness. Note that the total lag 
time includes not only the buffer length here, but also lag induced by the 
time stepper taking steps that are larger than the frame times. For maximum 
responsiveness you should keep the integrator step sizes limited to about 
100ms, or reduce the buffer length so that worst-case lag doesn't go much over
200ms. 
@param[in]      bufferLengthInSec
This is the target time length for the buffer. The actual length is the nearest
integer number of frames whose frame times add up closest to the request. If
you ask for a non-zero value, you will always get at least one frame in the
buffer. If you ask for zero, you'll get no buffering at all. To restore the
buffer length to its default value, pass in a negative number. 
@return A reference to this Visualizer so that you can chain "set" calls. **/
Visualizer& setDesiredBufferLengthInSec(Real bufferLengthInSec);
/** Get the current value of the desired buffer time length the Visualizer 
has been asked to use for smoothing the frame rate, or the default value
if none has been requested. The actual value will differ from this number
because the buffer must contain an integer number of frames. 
@see getActualBufferTime() to see the frame-rounded buffer length **/
Real getDesiredBufferLengthInSec() const;
/** Get the actual length of the real time frame buffer in seconds, which
may differ from the requested time because the buffer contains an integer
number of frames. **/
Real getActualBufferLengthInSec() const;
/** Get the actual length of the real time frame buffer in number of frames. **/
int getActualBufferLengthInFrames() const;

/** Add a new input listener to this Visualizer, methods of which will be
called when the GUI detects user-driven events like key presses, menu picks, 
and slider or mouse moves. See Visualizer::InputListener for more 
information. The Visualizer takes over ownership of the supplied \a listener 
object and deletes it upon destruction of the Visualizer; don't delete it 
yourself.
@return An integer index you can use to find this input listener again. **/
int addInputListener(InputListener* listener);
/** Return the count of input listeners added with addInputListener(). **/
int getNumInputListeners() const;
/** Return a const reference to the i'th input listener. **/
const InputListener& getInputListener(int i) const;
/** Return a writable reference to the i'th input listener. **/
InputListener& updInputListener(int i);

/** Add a new frame controller to this Visualizer, methods of which will be
called just prior to rendering a frame for the purpose of simulation-controlled
camera positioning and other frame-specific effects. 
See Visualizer::FrameController for more information. The Visualizer takes 
over ownership of the supplied \a controller object and deletes it upon 
destruction of the Visualizer; don't delete it yourself. 
@return An integer index you can use to find this frame controller again. **/
int addFrameController(FrameController* controller);
/** Return the count of frame controllers added with addFrameController(). **/
int getNumFrameControllers() const;
/** Return a const reference to the i'th frame controller. **/
const FrameController& getFrameController(int i) const;
/** Return a writable reference to the i'th frame controller. **/
FrameController& updFrameController(int i);

/**@}**/


/** @name               Frame drawing methods
These are used to report simulation frames to the Visualizer. Typically
the report() method will be called from a Reporter invoked by a TimeStepper, 
but it can also be useful to invoke directly to show preliminary steps in a
simulation, to replay saved States later, and to display frames when using
an Integrator directly rather than through a TimeStepper.

How frames are handled after they have been reported depends on the specific 
method called, and on the Visualizer's current Mode. **/

/**@{**/
/** Report that a new simulation frame is available for rendering. Depending
on the current Visualizer::Mode, handling of the frame will vary:

@par PassThrough
All frames will be rendered, but the calling thread (that is, the simulation) 
may be blocked if the next frame time has not yet been reached or if the 
renderer is unable to keep up with the rate at which frames are being supplied 
by the simulation.

@par Sampling 
The frame will be rendered immediately if the next sample time has been reached
or passed, otherwise the frame will be ignored and report() will return 
immediately.

@par RealTime
Frames are queued to smooth out the time stepper's variable time steps. The 
calling thread may be blocked if the buffer is full, or if the simulation time
is too far ahead of real time. Frames will be dropped if they come too 
frequently; only the ones whose simulated times are at or near a frame time 
will be rendered. Frames that come too late will be queued for rendering as 
soon as possible, and also reset the expected times for subsequent frames so 
that real time operation is restored. **/
void report(const State& state) const;

/** In RealTime mode there will typically be frames still in the buffer at
the end of a simulation.\ This allows you to wait while the buffer empties. 
When this returns, all frames that had been supplied via report() will have
been sent to the renderer and the buffer will be empty. Returns immediately
if not in RealTime mode, if there is no buffer, or if the buffer is already
empty. **/
void flushFrames() const;

/** This method draws a frame unconditionally without queuing or checking
the frame rate. Typically you should use the report() method instead, and
let the the internal queuing and timing system decide when to call 
drawFrameNow(). **/
void drawFrameNow(const State& state) const;
/**@}**/


/** @name                  Scene-building methods
These methods are used to add permanent elements to the scene being displayed
by the Visualizer. Once added, these elements will contribute to every frame.
Calling one of these methods requires writable (non-const) access to the 
Visualizer object; you can't call them from within a FrameController object.
Note that adding DecorationGenerators does allow different
geometry to be produced for each frame; however, once added a 
DecorationGenerator will be called for \e every frame generated. **/
/**@{**/

/** Add a new pull-down menu to the VisualizerGUI's display. A label
for the pull-down button is provided along with an integer identifying the
particular menu. A list of (string,int) pairs defines the menu and submenu 
item labels and associated item numbers. The item numbers must be unique 
across the entire menu and all its submenus. The strings have a pathname-like 
syntax, like "submenu/item1", "submenu/item2", "submenu/lowermenu/item1", etc.
that is used to define the pulldown menu layout. 
@param title    the title to display on the menu's pulldown button
@param id       an integer value >= 0 that uniquely identifies this menu
@param items    item names, possibly with submenus as specified above, with
                associated item numbers 
When a user picks an item on a menu displayed in the VisualizerGUI, that 
selection is delievered to the simulation application via an InputListener
associated with this Visualizer. The selection will be identified by
(\a id, itemNumber) pair. 
@return A reference to this Visualizer so that you can chain "add" and
"set" calls. **/
Visualizer& addMenu(const String& title, int id, 
                   const Array_<std::pair<String, int> >& items);

/** Add a new slider to the VisualizerGUI's display.
@param title    the title to display next to the slider
@param id       an integer value that uniquely identifies this slider
@param min      the minimum value the slider can have
@param max      the maximum value the slider can have
@param value    the initial value of the slider, which must be between 
                min and max 
When a user moves a slider displayed in the VisualizerGUI, the new value 
is delievered to the simulation application via an InputListener associated 
with this Visualizer. The slider will be identified by the \a id supplied
here. 
@return A reference to this Visualizer so that you can chain "add" and
"set" calls. **/
Visualizer& addSlider(const String& title, int id, Real min, Real max, Real value);

/** Add an always-present, body-fixed piece of geometry like the one passed in,
but attached to the indicated body. The supplied transform is applied on top of
whatever transform is already contained in the supplied geometry, and any body 
index stored with the geometry is ignored. 
@return An integer index you can use to find this decoration again. **/
int addDecoration(MobilizedBodyIndex mobodIx, const Transform& X_BD, 
                  const DecorativeGeometry& geometry);
/** Return the count of decorations added with addDecoration(). **/
int getNumDecorations() const;
/** Return a const reference to the i'th decoration. **/
const DecorativeGeometry& getDecoration(int i) const;
/** Return a writable reference to the i'th decoration. This is allowed for
a const %Visualizer since it is just a decoration. **/
DecorativeGeometry& updDecoration(int i) const;

/** Add an always-present rubber band line, modeled after the DecorativeLine 
supplied here. The end points of the supplied line are ignored, however: at 
run time the spatial locations of the two supplied stations are calculated and 
used as end points. 
@return An integer index you can use to find this rubber band line again. **/
int addRubberBandLine(MobilizedBodyIndex b1, const Vec3& station1,
                      MobilizedBodyIndex b2, const Vec3& station2,
                      const DecorativeLine& line);
/** Return the count of rubber band lines added with addRubberBandLine(). **/
int getNumRubberBandLines() const;
/** Return a const reference to the i'th rubber band line. **/
const DecorativeLine& getRubberBandLine(int i) const;
/** Return a writable reference to the i'th rubber band line. This is allowed
for a const %Visualizer since it is just a decoration. **/
DecorativeLine& updRubberBandLine(int i) const;

/** Add a DecorationGenerator that will be invoked to add dynamically generated
geometry to each frame of the the scene. The Visualizer assumes ownership of the 
object passed to this method, and will delete it when the Visualizer is 
deleted. 
@return An integer index you can use to find this decoration generator 
again. **/
int addDecorationGenerator(DecorationGenerator* generator);
/** Return the count of decoration generators added with 
addDecorationGenerator(). **/
int getNumDecorationGenerators() const;
/** Return a const reference to the i'th decoration generator. **/
const DecorationGenerator& getDecorationGenerator(int i) const;
/** Return a writable reference to the i'th decoration generator. **/
DecorationGenerator& updDecorationGenerator(int i);
/**@}**/

/** @name                Frame control methods
These methods can be called prior to rendering a frame to control how the 
camera is positioned for that frame. These can be invoked from within a
FrameController object for runtime camera control and other effects. **/
/**@{**/

/** Set the transform defining the position and orientation of the camera.

@param[in]   X_GC   This is the transform giving the pose of the camera's 
                    frame C in the ground frame G; see below for a precise
                    description.

Our camera uses a right-handed frame with origin at the image location,
with axes oriented as follows: the x axis is to the right, the y axis is the 
"up" direction, and the z axis is the "back" direction; that is, the camera is 
looking in the -z direction. If your simulation coordinate system is different,
such as the common "virtual world" system where ground is the x-y plane 
(x right and y "in") and z is up, be careful to account for that when 
positioning the camera. 

For example, in the virtual world coordinate system, setting \a X_GC to 
identity would put the camera at the ground origin with the x axis as expected,
but the camera would be looking down (your -z) with the camera's "up" direction
aligned with your y. In this case to make the camera look in the y direction 
with up in z, you would need to rotate it +90 degrees about the x axis:
@code
Visualizer viz;
// ...

// Point camera along Ground's y axis with z up, by rotating the camera
// frame's z axis to align with Ground's -y.
viz.setCameraTransform(Rotation(Pi/2, XAxis));
@endcode **/
const Visualizer& setCameraTransform(const Transform& X_GC) const;

/** Move the camera forward or backward so that all geometry in the scene is 
visible. **/
const Visualizer& zoomCameraToShowAllGeometry() const;

/** Rotate the camera so that it looks at a specified point.
@param point        the point to look at
@param upDirection  a direction which should point upward as seen by the camera
**/
const Visualizer& pointCameraAt(const Vec3& point, const Vec3& upDirection) const;

/** Set the camera's vertical field of view, measured in radians. **/
const Visualizer& setCameraFieldOfView(Real fov) const;

/** Set the distance from the camera to the near and far clipping planes. **/
const Visualizer& setCameraClippingPlanes(Real nearPlane, Real farPlane) const;

/** Change the value currently shown on one of the sliders. 
@param slider       the id given to the slider when created
@param value        a new value for the slider; if out of range it will 
                    be at one of the extremes **/
const Visualizer& setSliderValue(int slider, Real value) const;

/** Change the allowed range for one of the sliders. 
@param slider   the id given to the slider when created
@param newMin   the new lower limit on the slider range, <= newMax   
@param newMax   the new upper limit on the slider range, >= newMin
The slider's current value remains unchanged if it still fits in the
new range, otherwise it is moved to the nearest limit. **/
const Visualizer& setSliderRange(int slider, Real newMin, Real newMax) const;


/**@}**/

/** @name            Methods for debugging and statistics **/
/**@{**/
/** Dump statistics to the given ostream (for example, std::cout). **/
void dumpStats(std::ostream& o) const;
/** Reset all statistics to zero. **/
void clearStats();
/**@}**/

/** @name                       Internal use only **/
/**@{**/
const Array_<InputListener*>&   getInputListeners() const;
const Array_<FrameController*>& getFrameControllers() const;
const MultibodySystem&          getSystem() const;
int getRefCount() const;
/**@}**/

class Impl;
//--------------------------------------------------------------------------
                                private:
explicit Visualizer(Impl* impl);
Impl* impl;

const Impl& getImpl() const {assert(impl); return *impl;}
Impl&       updImpl()       {assert(impl); return *impl;}
friend class Impl;
};

/** This abstract class represents an object that will be invoked by the
Visualizer just prior to rendering each frame. You can use this to call any
of the const (runtime) methods of the Visualizer, typically to control the 
camera, and you can also add some geometry to the scene, print messages to 
the console, and so on. **/
class SimTK_SIMBODY_EXPORT Visualizer::FrameController {
public:
    /** The Visualizer is just about to generate and render a frame 
    corresponding to the given State. 
    @param[in]          viz     
        The Visualizer that is doing the rendering.
    @param[in]          state   
        The State that is being used to generate the frame about to be
        rendered by \a viz.
    @param[in,out]      geometry 
        DecorativeGeometry being accumulated for rendering in this frame;
        be sure to \e append if you have anything to add.
    **/
    virtual void generateControls(const Visualizer&           viz, 
                                  const State&                state,
                                  Array_<DecorativeGeometry>& geometry) = 0;

    /** Destructor is virtual; be sure to override it if you have something
    to clean up at the end. **/
    virtual ~FrameController() {}
};

/** OBSOLETE: This provides limited backwards compatibility with the old
VTK Visualizer that is no longer supported.\ Switch to Visualizer instead. **/
typedef Visualizer VTKVisualizer;

} // namespace SimTK

#endif // SimTK_SIMBODY_VISUALIZER_H_
