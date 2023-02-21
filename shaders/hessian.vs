// Vertex shader that computes the Hessian matrix for each vertex.
// Doesn't actually run I think, written by GPT.
#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

const float epsilon = 1e-6;

//position, normal, u, v
//utilize out parameters
mat3 computeHessian(vec3 p, vec3 n, vec3 u, vec3 v)
{
    vec3 du = dFdx(p); //dFdx is built-in, computes by fragment in x direction -> ONLY AVAILABLE IN FRAGMENT SHADER
    vec3 dv = dFdy(p);
    //https://stackoverflow.com/questions/16365385/explanation-of-dfdx

    mat3 H = mat3(0.0);
    H[0][0] = dot(du, du);
    H[0][1] = dot(du, dv);
    H[1][0] = H[0][1];
    H[1][1] = dot(dv, dv);
    H[2][0] = dot(n, du);
    H[0][2] = H[2][0];
    H[2][1] = dot(n, dv);
    H[1][2] = H[2][1];

    return H;
}
void eigen33(mat3 A, out vec3 eigenvals, out mat3 eigenvecs)
{
    const int max_iterations = 32;
    const float eps = 1e-6;

    vec3 row1 = A[0];
    vec3 row2 = A[1];
    vec3 row3 = A[2];

    for (int i = 0; i < max_iterations; i++) {
        mat3 Q = mat3(1.0);
        mat3 R = mat3(0.0);

        vec3 a = vec3(length(row1), 0.0, 0.0);
        if (a.x > eps) {
            vec3 u = normalize(row1 - a);
            vec3 v = cross(u, row1);
            Q = mat3(u, v, row1);
            R = transpose(Q) * A;
            A = R * Q;
        }

        vec3 b = vec3(length(row2), 0.0, 0.0);
        if (b.x > eps) {
            vec3 u = normalize(row2 - b);
            vec3 v = cross(u, row2);
            Q = mat3(u, v, row2);
            R = transpose(Q) * A;
            A = R * Q;
        }

        vec3 c = vec3(length(row3), 0.0, 0.0);
        if (c.x > eps) {
            vec3 u = normalize(row3 - c);
            vec3 v = cross(u, row3);
            Q = mat3(u, v, row3);
            R = transpose(Q) * A;
            A = R * Q;
        }

        if (length(A[0] - row1) < eps && length(A[1] - row2) < eps && length(A[2] - row3) < eps) {
            break;
        }

        row1 = A[0];
        row2 = A[1];
        row3 = A[2];
    }

    eigenvecs = mat3(row1, row2, row3);
    eigenvals = vec3(A[0][0], A[1][1], A[2][2]);
}

void main()
{
    vec3 worldPos = (modelMatrix * vec4(position, 1.0)).xyz;
    vec3 worldNormal = normalize((modelMatrix * vec4(normal, 0.0)).xyz);

    vec3 tangent = cross(worldNormal, vec3(1.0, 0.0, 0.0));
    if (length(tangent) < epsilon) {
        tangent = cross(worldNormal, vec3(0.0, 1.0, 0.0));
    }
    vec3 bitangent = cross(worldNormal, tangent);

    mat3 TBN = mat3(tangent, bitangent, worldNormal);
    mat3 invTBN = transpose(TBN);

    vec3 du = normalize(invTBN * dFdx(worldPos));
    vec3 dv = normalize(invTBN * dFdy(worldPos));

    mat3 H = computeHessian(worldPos, worldNormal, du, dv);
    mat3 eigenvecs;
    vec3 eigenvals;
    eigen33(H, eigenvals, eigenvecs);

    // Store the eigenvectors in a buffer object for use by the CPU.
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
}

/*
The "eigen33" function returns the eigenvalues and eigenvectors by passing them as output parameters to the function. In the function signature, the eigenvals and eigenvecs parameters are declared as out parameters, which means that they will be modified by the function and their new values will be returned to the caller.

The eigenvals parameter is a vec3 variable that will hold the three eigenvalues of the input 3x3 matrix. The eigenvalues are stored as a vector of length 3 because a 3x3 matrix can have at most three distinct eigenvalues.

The eigenvecs parameter is a mat3 variable that will hold the eigenvectors of the input 3x3 matrix. The eigenvectors are stored as the columns of the 3x3 matrix, with the first column containing the eigenvector corresponding to the first eigenvalue, the second column containing the eigenvector corresponding to the second eigenvalue, and so on.

The function calculates the eigenvalues and eigenvectors using the QR decomposition method, which is an iterative algorithm that computes the eigenvalues and eigenvectors of a matrix by repeatedly transforming it into an upper-triangular form. In each iteration, the function applies a series of Givens rotations to the matrix to zero out the subdiagonal elements, while accumulating the transformation matrices into a separate matrix that represents the eigenvectors. Once the matrix is in upper-triangular form, the eigenvalues can be read off from the diagonal elements, and the eigenvectors can be obtained by applying the accumulated transformation matrix to the identity matrix.
*/
