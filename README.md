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

Install this package into <OpenSim install directory>/plugins, where
  <OpenSim install directory> may be something like C:/OpenSim 3.2.
  
To use this plugin in the OpenSim GUI:

- Add <OpenSim install directory>/plugins to your PATH.
- Open the OpenSim GUI, and load the plugin via Tools > User Plugins.

To use this plugin with OpenSim command line tools on Windows:

    > cmc -S setup_file.xml -L <OpenSim install directory/plugins/osimMuscleMetabolicsProbes
    
On UNIX:

    $ cmc -S setup_file.xml -L <OpenSim install directory/plugins/libosimMuscleMetabolicsProbes

