# ApparentRidges

Implementation of [Apparent Ridges for Line Drawings](https://people.csail.mit.edu/tjudd/apparentridges.html).

Implemented in OpenGL with C++, using OpenGL compute shaders for computations on the GPU.

![]()

*Apparent Ridges for Line Drawings* presents an effective method line drawing rendering of meshes while effectively conveying mesh topology through drawing *apparent ridges*.

**Apparent ridges** are defined as the groups of points that maximize ** *view-dependent curvature* **.

## Method

The principle question in line drawing is where the lines should be drawn to best represent and express the mesh geometry.

The paper presents a rule for line drawing to do this as to **draw a line when the surface normal is changing at a locally maximal rate with respect to image position**.

These lines are defined as ***apparent ridges***. They depend on the curvature of the mesh itself, as well as its projection to the view plane.

### Estimating Curvatures and Their Derivatives on Triangle Meshes

To compute the view-dependent curvature over each point, we must first estimate the original view independent curvature of our 3D mesh.

Using the method from *Estimating Curvatures and Their Derivatives on Triangle Meshes* [Rusinkiewicz 2004], we can estimate the shape operator at every vertex by fitting it to the one-ring neighborhood of every vertex. For every triangle face, with finite differences of the normals on the vertices of the face, we can compute an estimate to the derivative of the surface normal in the direction along the edge. Then, with the linear constraints produced by the three edges and estimates to normal derivatives, we can estimate the **shape operator** per vertex using linear least squares. With the shape operator matrix estimated, we can derive the principal curvatures and principal directions of the surface at each vertex by calculating the eigenvalues and eigenvectors of the shape operator.

<img width="360" alt="curvatureTensorEstimation" src="https://user-images.githubusercontent.com/38942504/226597274-37034f2c-1574-4e13-bf74-98fd5c472e6b.png">

### View-dependent Curvature

### Apparent Ridges

### Implementation on Meshes

## References

[Apparent Ridges for Line Drawings](https://people.csail.mit.edu/tjudd/apparentridges.html) [Judd et al. 2007]

[Estimating Curvatures and Their Derivatives on Triangle Meshes](https://gfx.cs.princeton.edu/pubs/Rusinkiewicz_2004_ECA/curvpaper.pdf)  [Rusinkiewicz 2004]

[RTSC](https://rtsc.cs.princeton.edu/)

[TriMesh2](https://gfx.cs.princeton.edu/proj/trimesh2/)


