# ApparentRidges
Implementation of [Apparent Ridges for Line Drawings](https://people.csail.mit.edu/tjudd/apparentridges.html).

Implemented in OpenGL with C++, using OpenGL compute shaders for computations on the GPU.

(Original paper was demonstrated with CPU run code.)
![]()

*Apparent Ridges for Line Drawings* presents an effective method of conveying mesh topology by rendering line drawings of 3D meshes through drawing *apparent ridges*.
**Apparent ridges** are defined as the group of points that maximize ** *view-dependent curvature***.

## Method

### Estimating Curvatures and Their Derivatives on Triangle Meshes

To compute the view-dependent curvature over each point, we must first estimate the original view independent curvature of our 3D mesh.

Estimating Curvatures and Their Derivatives on Triangle Meshes [Rusinkiewicz 2004]

##
##

## References

Apparent Ridges for Line Drawings [Judd et al. 2007]

Estimating Curvatures and Their Derivatives on Triangle Meshes [Rusinkiewicz 2004]

RTSC

TriMesh2

