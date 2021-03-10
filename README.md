[![Build Status](https://travis-ci.com/I2PC/xmipp.svg?branch=devel)](https://travis-ci.com/I2PC/xmipp)
<!---  [![Quality Gate](https://sonarcloud.io/api/project_badges/measure?project=Xmipp&metric=alert_status)](https://sonarcloud.io/dashboard?id=Xmipp)
[![Technical debt](https://sonarcloud.io/api/project_badges/measure?project=Xmipp&metric=sqale_index)](https://sonarcloud.io/component_measures?id=Xmipp&metric=sqale_index)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=Xmipp&metric=bugs)](https://sonarcloud.io/project/issues?id=Xmipp&resolved=false&types=BUG)
--->
# Xmipp

Welcome to Xmipp. Xmipp is a suite of image processing programs, primarily aimed at single-particle 3D electron microscopy.


## 1. Getting started
Xmipp can be installed and used in two general ways:
* Installation to use Xmipp in Scipion (Recommended installation). This installation can be split in two.
** Installation of Xmipp in Scipion for users (see section 2.1)
** Installation of Xmipp in Scipion for developers (see section 2.2)
* Standalone installation only Xmipp without Scipion - for advanced developers (see section 4). 

## 2.  Installation to use Xmipp in Scipion

Before installing Xmipp we would like highlight the troubleshooting section, where most common drawbacks are shown and their solutions.

*  **2.1. Installation of Xmipp in Scipion for users.**

If you are not a developer this should be the installation to follow. As requirement you need to have [Scipion](https://scipion-em.github.io/docs/docs/scipion-modes/how-to-install.html) installed in your computer. Once you have Scipion installed, Xmipp can be easily installed using the [Plugin manager](https://scipion-em.github.io/docs/docs/user/plugin-manager.html).


*  **2.2.  Installation of Xmipp in Scipion for developers.**

Start by cloning the repository from GitHub and go there (this action requires git installed in your computer).
```
git clone https://github.com/I2PC/xmipp xmipp-bundle
cd xmipp-bundle
```
Execute the xmipp script
  ```
  ./xmipp
  ```
  [brancn]is an optional argument to specify the branch that will be download. If the argument [branch] is ommited, then the devel branch will be installed (by default).
  
  Secondly, install the 'scipion-em-xmipp' plugin in development mode. This plugin was downloaded when xmipp was cloned in the first step of this intallation. IT must be highlighted that Scipion is needed in this step, and scipion3 should be visible in the path (check the [Scipion's installation guide](https://scipion-em.github.io/docs/docs/scipion-modes/how-to-install.html#launching-scipion3)).
  ```
  scipion3 installp -p $PWD/src/scipion-em-xmipp --devel
  ```
  
  Finally, compile Xmipp under Scipion's environ
  ```
  scipion3 installb xmippDev -j 8
  ```
  where `-j 8` indicates that 8 cores will be used to compile Xmipp.
  
  Now Xmipp should be visible in Scipion and ready to use.
  

## Xmipp as a standalone bundle (for developers)

Start by cloning the repository from GitHub and go there (this action requires git installed in your computer).
```
git clone https://github.com/I2PC/xmipp xmipp-bundle
cd xmipp-bundle
```
Now, execute the xmipp script to install and compile xmipp. This script will ask the path of some dependencies. If these dependencies are not provided xmipp will try to find them for you. If this script fails go to Troubleshooting section.
  ```
  ./xmipp
  ```
 If you consider that the compilation is too slow, you can compile using more processors with the command line
 ```
  ./xmipp -j 4
  ```
where -j determine the number of processors, in this case we chose 4 processors to compile.
  
  Now Xmipp should be ready to use. To use Xmipp just run the xmipp.bashrc script in the build folder
  
 ```
 source build/xmipp.bashrc
 ```
 Thus, xmipp commands will be ready to be used, for instance
 
 ```
 xmipp_image_convert
 ```


## Troubleshooting

* Dependencies: Next libraries are required to install Xmipp.

  ```
  sudo apt-get install gcc-5 g++-5 cmake openjdk-8-jdk libxft-dev libssl-dev libxext-dev libxml2-dev libreadline7 libquadmath0 libxslt1-dev libopenmpi-dev openmpi-bin libxss-dev libgsl0-dev libx11-dev gfortran libfreetype6-dev scons libfftw3-dev libopencv-dev curl git
  ```

* Check the [**Xmipp configuration guide**](https://github.com/I2PC/xmipp/wiki/Xmipp-configuration-(version-20.07)). 


  You can see the whole usage of the script with `./xmipp --help`. The most useful options are `br=branch_name` to select a specific branch to be checkout-ed, and `N=#processors` to use for the build (they must be in combination with the `all` option). Follow the next recipe (also read `./xmipp help`):
> use `scipion3 run ./xmipp [options]` if you are installing Xmipp under Scipion



### Additional Information for developers: Detailed installation

Next command shoule be launched by the xmipp script, however, some developers preffer to know more details about the installer.
```
git clone https://github.com/I2PC/xmipp xmipp-bundle  # This clones the main Xmipp repo into xmipp-bundle directory
cd xmipp-bundle
./xmipp get_devel_sources [branch]                    # This downloads the rest of Xmipp repos in a certain branch
./xmipp config                                        # This configures the Xmipp installation according to the system
./xmipp check_config                                  # This checks the configuration set
./xmipp get_dependencies                              # This downloads the dependencies that Xmipp needs according to the configuration
./xmipp compile [N]                                   # This compiles Xmipp using N processors
./xmipp install [directory]                           # This installs Xmipp to a certain directory

# Optionally, Xmipp plugin for Scipion can be installed under the python/environ of Scipion
pip install -e src/scipion-em-xmipp                   # CHECK the ENVIRON that are present in the session!!
```

_The `./xmipp` and `./xmipp all` commands make the same than the receip above at once (except for the first and last commands) by taking the default values_




