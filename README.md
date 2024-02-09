# simulink-savedata-block
Simulink&reg; block that stores data in memory and saves them to a file at the end of the simulation.

[![View simulink-savedata-block on File Exchange](https://www.mathworks.com/matlabcentral/images/matlab-file-exchange.svg)](https://www.mathworks.com/matlabcentral/fileexchange/9986-simulink-savedata-block)

[![Open in MATLAB Online](https://www.mathworks.com/images/responsive/global/open-in-matlab-online.svg)](https://matlab.mathworks.com/open/github/v1?repo=giampy1969/simulink-savedata-block)

This block is similar to the "To File" original simulink block, except for the following points:

1) It does not access the file system during the simulation, in fact it collects data in memory and saves them to file only when the simulation is over. This is important for real time systems, where normally access to the file system by processes running in real time has to be avoided.

2) It can save any type of data instead of just doubles.

3) It has a lot of flexibility in saving the data, different format and precisons can be selected form the block mask.

4) When using the %d, %f or %e options, the file is an ASCII file that can be opened with any text editor as well as easily loaded into matlab or excel.

This code is derived from an original s-function written by G.Campa and S.Gururajan for simulink schemes running in real time under RTAI-Linux.
