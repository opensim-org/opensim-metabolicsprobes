Here's how you can run this example from the command line:

On Windows:

    > analyze -S subject01_Setup_Analyze_Metabolics.xml -L <OpenSim install directory>/plugins/osimMuscleMetabolicsProbes
    
On UNIX:

    $ analyze -S subject01_Setup_Analyze_Metabolics.xml -L <OpenSim install directory>/plugins/libosimMuscleMetabolicsProbes
    
This will generate a file ResultsAnalyze/subject01_walk1_ProbeReporter_probes.sto.
You can check that the data in that file is the same as that in 
ReferenceResultsAnalyze/subject01_walk1_ProbeReporter_probes.sto.