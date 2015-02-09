OpenSim muscle metabolics probes
================================

Tom Uchida (@tkuchida) has made numerous changes to OpenSim's muscle metabolics
probes.  This plugin contains his changes so that people can use these
updated probes before the next version of OpenSim is released.

In this plugin, the probes have been renamed (e.g.,
Umberger2010MuscleMetabolicsProbe is now
UchidaUmberger2010MuscleMetabolicsProbe) to avoid a name conflict with the
existing probes. In the next release of OpenSim, his changes will be
incorporated into the original Umberger2010MuscleMetabolicsProbe. This means
that you need to rename the probe in your XML files.

DISCLAIMER: These probes are in development, and their interface may change in
the future.

Usage
-----

- Place the library files into <OpenSim install directory>/plugins, where
  <OpenSim install directory> may be something like C:/OpenSim 3.2.
  
- Add metabolics probes to your model file (osim). The examples folder contains
a model with metabolics probes (in the ProbeSet).

- Add a ProbeReporter to the AnalysisSet for the tool you'll be using.
You can use the probes in CMC (by adding a ProbeReporter to the 
AnalysisSet in the CMC setup file), but this can substantially increase
the runtime of CMC. We advise using the probes afterward in the AnalyzeTool.
An example AnalyzeTool setup file (with a ProbeReporter) is in the examples folder.
Note that the AnalyzeTool must contain a ControlSetController that
uses CMC's excitations. Otherwise, the probe output will be incorrect
(the probes depend on excitations). This is also shown in the examples folder.

- Run the tool! In the OpenSim GUI:

    - Add <OpenSim install directory>/plugins to your PATH, where
      <OpenSim install directory> is probably C:/OpenSim 3.1 or C:/OpenSim 3.2.
    - Open the OpenSim GUI, and load the plugin via Tools > User Plugins.
    - Run the tool (CMC or Analyze) using the setup file that contains the ProbeReporter.

To use this plugin with OpenSim command line tools on Windows:

    > analyze -S setup_file.xml -L <OpenSim install directory>/plugins/osimMuscleMetabolicsProbes
    
On UNIX:

    $ analyze -S setup_file.xml -L <OpenSim install directory>/plugins/libosimMuscleMetabolicsProbes

