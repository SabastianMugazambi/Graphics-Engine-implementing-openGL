
#define BUFFER_OFFSET(bytes) ((GLubyte *)NULL + (bytes))

/*** Creating and destroying ***/

/* Feel free to read the struct's members, but don't write them, except through
the accessors below such as meshSetTriangle, meshSetVertex. */
typedef struct meshMesh meshMesh;
struct meshMesh {
  GLuint triNum, vertNum, attrDim;
  GLuint *tri;    /* triNum * 3 GLuints */
  GLdouble *vert; /* vertNum * attrDim GLdoubles */
};

/* Initializes a mesh with enough memory to hold its triangles and vertices.
Does not actually fill in those triangles or vertices with useful data. When
you are finished with the mesh, you must call meshDestroy to deallocate its
backing resources. */
int meshInitialize(meshMesh *mesh, GLuint triNum, GLuint vertNum,
                   GLuint attrDim) {
  mesh->tri = (GLuint *)malloc(triNum * 3 * sizeof(GLuint) +
                               vertNum * attrDim * sizeof(GLdouble));
  if (mesh->tri != NULL) {
    mesh->vert = (GLdouble *)&(mesh->tri[triNum * 3]);
    mesh->triNum = triNum;
    mesh->vertNum = vertNum;
    mesh->attrDim = attrDim;
  }
  return (mesh->tri == NULL);
}

void meshPointInitialize(meshMesh *mesh, GLuint vertNum, GLuint attrDim) {

  mesh->vert = (GLdouble *)malloc(vertNum * attrDim * sizeof(GLdouble));
  mesh->vertNum = vertNum;
  mesh->attrDim = attrDim;

}

/* Sets the trith triangle to have vertex indices i, j, k. */
void meshSetTriangle(meshMesh *mesh, GLuint tri, GLuint i, GLuint j, GLuint k) {
  if (tri < mesh->triNum) {
    mesh->tri[3 * tri] = i;
    mesh->tri[3 * tri + 1] = j;
    mesh->tri[3 * tri + 2] = k;
  }
}

/* Returns a pointer to the trith triangle. For example:
        GLuint *triangle13 = meshGetTrianglePointer(&mesh, 13);
        printf("%d, %d, %d\n", triangle13[0], triangle13[1], triangle13[2]); */
GLuint *meshGetTrianglePointer(meshMesh *mesh, GLuint tri) {
  if (tri < mesh->triNum)
    return &mesh->tri[tri * 3];
  else
    return NULL;
}

/* Sets the vertth vertex to have attributes attr. */
void meshSetVertex(meshMesh *mesh, GLuint vert, GLdouble attr[]) {
  GLuint k;
  if (vert < mesh->vertNum)
    for (k = 0; k < mesh->attrDim; k += 1)
      mesh->vert[mesh->attrDim * vert + k] = attr[k];
}

/* Returns a pointer to the vertth vertex. For example:
        GLdouble *vertex13 = meshGetVertexPointer(&mesh, 13);
        printf("x = %f, y = %f\n", vertex13[0], vertex13[1]); */
GLdouble *meshGetVertexPointer(meshMesh *mesh, GLuint vert) {
  if (vert < mesh->vertNum)
    return &mesh->vert[vert * mesh->attrDim];
  else
    return NULL;
}

/* Deallocates the resources backing the mesh. This function must be called
when you are finished using a mesh. */
void meshDestroy(meshMesh *mesh) { free(mesh->tri); }
//void meshPtcDestroy(meshMesh *mesh) { free(mesh->vert); }

/*** OpenGL ***/

/* Feel free to read from this struct's members, but don't write to them,
except through accessor functions. */
typedef struct meshGLMesh meshGLMesh;
struct meshGLMesh {
  GLuint triNum, vertNum, attrDim, vaoNum, attrNum;
  GLuint *attrDims;
  GLuint *vaos;
  GLuint buffers[2];
};

/* attrLocs is meshGL->attrNum locations in the active shader program. index is
an integer between 0 and meshGL->voaNum - 1, inclusive. This function
initializes the VAO at that index in the meshGL's array of VAOs, so that the
VAO can render using those locations. */
void meshGLVAOInitialize(meshGLMesh *meshGL, GLuint index, GLint attrLocs[]) {
  glBindVertexArray(meshGL->vaos[index]);
  /* Make sure the intended shader program is active. (In a serious
  application, we might switch among several shaders rapidly.) Connect our
  attribute array to the attributes in the vertex shader. */

  GLint offset_num = 0;

  for (GLuint i = 0; i < meshGL->attrNum; i++) {
    glEnableVertexAttribArray(attrLocs[i]);
  }

  glBindBuffer(GL_ARRAY_BUFFER, meshGL->buffers[0]);

  for (GLuint i = 0; i < meshGL->attrNum; i++) {
    GLuint attrDim = meshGL->attrDims[i];
    glVertexAttribPointer(attrLocs[i], attrDim, GL_DOUBLE, GL_FALSE,
                          meshGL->attrDim * sizeof(GLdouble),
                          BUFFER_OFFSET(offset_num * sizeof(GLdouble)));

    offset_num += attrDim;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshGL->buffers[1]);

  glBindVertexArray(0);

}

/* Initializes an OpenGL mesh from a non-OpenGL mesh. vaoNum is the number of
vertex array objects attached to this mesh storage. Typically vaoNum equals the
number of distinct shader programs that will need to draw the mesh. Returns 0
on success, non-zero on failure. */
int meshGLInitialize(meshGLMesh *meshGL, meshMesh *mesh, GLuint attrNum,
        GLuint attrDims[], GLuint vaoNum) {
    meshGL->attrDims = (GLuint *)malloc((attrNum + vaoNum) * sizeof(GLuint));
    if (meshGL->attrDims == NULL)
        return 1;
    for (int i = 0; i < attrNum; i += 1)
        meshGL->attrDims[i] = attrDims[i];
    meshGL->vaos = &meshGL->attrDims[attrNum];
    glGenVertexArrays(vaoNum, meshGL->vaos);
    meshGL->vaoNum = vaoNum;
    meshGL->attrNum = attrNum;
    meshGL->triNum = mesh->triNum;
    meshGL->vertNum = mesh->vertNum;
    meshGL->attrDim = mesh->attrDim;
    glGenBuffers(2, meshGL->buffers);
    glBindBuffer(GL_ARRAY_BUFFER, meshGL->buffers[0]);
    glBufferData(GL_ARRAY_BUFFER,
        meshGL->vertNum * meshGL->attrDim * sizeof(GLdouble),
        (GLvoid *)(mesh->vert), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshGL->buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshGL->triNum * 3 * sizeof(GLuint),
        (GLvoid *)(mesh->tri), GL_STATIC_DRAW);
    return 0;
}

/* Renders the already-initialized OpenGL mesh. attrDims is an array of length
attrNum. For each i, its ith entry is the dimension of the ith attribute
vector. Similarly, attrLocs is an array of length attrNum, giving the location
of the ith attribute in the active OpenGL shader program. */
void meshGLRender(meshGLMesh *meshGL, GLuint vaoIndex) {
  /* Make sure the intended shader program is active. (In a serious
  application, we might switch among several shaders rapidly.) Connect our
  attribute array to the attributes in the vertex shader. */
  glBindVertexArray(meshGL->vaos[vaoIndex]);
  glDrawElements(GL_TRIANGLES, meshGL->triNum * 3, GL_UNSIGNED_INT,
                 BUFFER_OFFSET(0));
  glBindVertexArray(0);
}

/* Deallocates the resources backing the initialized OpenGL mesh. */
void meshGLDestroy(meshGLMesh *meshGL) {
  glDeleteBuffers(2, meshGL->buffers);
  glDeleteVertexArrays(meshGL->vaoNum, meshGL->vaos);
  free(meshGL->attrDims);
}

/*** Convenience initializers: 2D ***/

/* Initializes a mesh to two triangles forming a rectangle of the given sides.
The four attributes are X, Y, S, T. Do not call meshInitialize separately; it
is called inside this function. Don't forget to call meshDestroy when done. */
int meshInitializeRectangle(meshMesh *mesh, GLdouble left, GLdouble right,
                            GLdouble bottom, GLdouble top) {
  GLuint error = meshInitialize(mesh, 2, 4, 2 + 2);
  if (error == 0) {
    meshSetTriangle(mesh, 0, 0, 1, 2);
    meshSetTriangle(mesh, 1, 0, 2, 3);
    GLdouble attr[4];
    vecSet(4, attr, left, bottom, 0.0, 0.0);
    meshSetVertex(mesh, 0, attr);
    vecSet(4, attr, right, bottom, 1.0, 0.0);
    meshSetVertex(mesh, 1, attr);
    vecSet(4, attr, right, top, 1.0, 1.0);
    meshSetVertex(mesh, 2, attr);
    vecSet(4, attr, left, top, 0.0, 1.0);
    meshSetVertex(mesh, 3, attr);
  }
  return error;
}

/* Initializes a mesh to sideNum triangles forming an ellipse of the given
center (x, y) and radii rx, ry. The four attributes are X, Y, S, T. Do not call
meshInitialize separately; it is called inside this function. Don't forget to
call meshDestroy when done. */
int meshInitializeEllipse(meshMesh *mesh, GLdouble x, GLdouble y, GLdouble rx,
                          GLdouble ry, GLuint sideNum) {
  GLuint i, error;
  GLdouble theta, cosTheta, sinTheta, attr[4] = {x, y, 0.5, 0.5};
  error = meshInitialize(mesh, sideNum, sideNum + 1, 2 + 2);
  if (error == 0) {
    meshSetVertex(mesh, 0, attr);
    for (i = 0; i < sideNum; i += 1) {
      meshSetTriangle(mesh, i, 0, i + 1, (i + 1) % sideNum + 1);
      theta = i * 2.0 * M_PI / sideNum;
      cosTheta = cos(theta);
      sinTheta = sin(theta);
      vecSet(4, attr, x + rx * cosTheta, y + ry * sinTheta,
             0.5 * cosTheta + 0.5, 0.5 * sinTheta + 0.5);
      meshSetVertex(mesh, i + 1, attr);
    }
  }
  return error;
}

/*** Convenience initializers: 3D ***/

/* Assumes that attributes 0, 1, 2 are XYZ. Assumes that the vertices of the
triangle are in counter-clockwise order when viewed from 'outside' the
triangle. Computes the outward-pointing unit normal vector for the triangle. */
void meshTrueNormal(GLdouble a[], GLdouble b[], GLdouble c[],
                    GLdouble normal[3]) {
  GLdouble bMinusA[3], cMinusA[3];
  vecSubtract(3, b, a, bMinusA);
  vecSubtract(3, c, a, cMinusA);
  vec3Cross(bMinusA, cMinusA, normal);
  vecUnit(3, normal, normal);
}

/* Assumes that attributes 0, 1, 2 are XYZ. Sets attributes n, n + 1, n + 2 to
flat-shaded normals. If a vertex belongs to more than triangle, then some
unspecified triangle's normal wins. */
void meshFlatNormals(meshMesh *mesh, GLuint n) {
  GLuint i, *tri;
  GLdouble *a, *b, *c, normal[3];
  for (i = 0; i < mesh->triNum; i += 1) {
    tri = meshGetTrianglePointer(mesh, i);
    a = meshGetVertexPointer(mesh, tri[0]);
    b = meshGetVertexPointer(mesh, tri[1]);
    c = meshGetVertexPointer(mesh, tri[2]);
    meshTrueNormal(a, b, c, normal);
    vecCopy(3, normal, &a[n]);
    vecCopy(3, normal, &b[n]);
    vecCopy(3, normal, &c[n]);
  }
}

/* Assumes that attributes 0, 1, 2 are XYZ. Sets attributes n, n + 1, n + 2 to
smooth-shaded normals. Does not do anything special to handle multiple vertices
with the same coordinates. */
void meshSmoothNormals(meshMesh *mesh, GLuint n) {
  GLuint i, *tri;
  GLdouble *a, *b, *c, normal[3] = {0.0, 0.0, 0.0};
  /* Zero the normals. */
  for (i = 0; i < mesh->vertNum; i += 1) {
    a = meshGetVertexPointer(mesh, i);
    vecCopy(3, normal, &a[n]);
  }
  /* For each triangle, add onto the normal at each of its vertices. */
  for (i = 0; i < mesh->triNum; i += 1) {
    tri = meshGetTrianglePointer(mesh, i);
    a = meshGetVertexPointer(mesh, tri[0]);
    b = meshGetVertexPointer(mesh, tri[1]);
    c = meshGetVertexPointer(mesh, tri[2]);
    meshTrueNormal(a, b, c, normal);
    vecAdd(3, normal, &a[n], &a[n]);
    vecAdd(3, normal, &b[n], &b[n]);
    vecAdd(3, normal, &c[n], &c[n]);
  }
  /* Normalize the normals. */
  for (i = 0; i < mesh->vertNum; i += 1) {
    a = meshGetVertexPointer(mesh, i);
    vecUnit(3, &a[n], &a[n]);
  }
}

/* Builds a mesh for a parallelepiped (box) of the given size. The attributes
are XYZ position, ST texture, and NOP unit normal vector. The normals are
discontinuous at the edges (flat shading, not smooth). To facilitate this, some
vertices have equal XYZ but different NOP, for 24 vertices in all. Don't forget
to meshDestroy when finished. */
int meshInitializeBox(meshMesh *mesh, GLdouble left, GLdouble right,
                      GLdouble bottom, GLdouble top, GLdouble base,
                      GLdouble lid) {
  GLuint error = meshInitialize(mesh, 12, 24, 3 + 2 + 3);
  if (error == 0) {
    /* Make the triangles. */
    meshSetTriangle(mesh, 0, 0, 2, 1);
    meshSetTriangle(mesh, 1, 0, 3, 2);
    meshSetTriangle(mesh, 2, 4, 5, 6);
    meshSetTriangle(mesh, 3, 4, 6, 7);
    meshSetTriangle(mesh, 4, 8, 10, 9);
    meshSetTriangle(mesh, 5, 8, 11, 10);
    meshSetTriangle(mesh, 6, 12, 13, 14);
    meshSetTriangle(mesh, 7, 12, 14, 15);
    meshSetTriangle(mesh, 8, 16, 18, 17);
    meshSetTriangle(mesh, 9, 16, 19, 18);
    meshSetTriangle(mesh, 10, 20, 21, 22);
    meshSetTriangle(mesh, 11, 20, 22, 23);
    /* Make the vertices after 0, using vertex 0 as temporary storage. */
    GLdouble *v = mesh->vert;
    vecSet(8, v, right, bottom, base, 1.0, 0.0, 0.0, 0.0, -1.0);
    meshSetVertex(mesh, 1, v);
    vecSet(8, v, right, top, base, 1.0, 1.0, 0.0, 0.0, -1.0);
    meshSetVertex(mesh, 2, v);
    vecSet(8, v, left, top, base, 0.0, 1.0, 0.0, 0.0, -1.0);
    meshSetVertex(mesh, 3, v);
    vecSet(8, v, left, bottom, lid, 0.0, 0.0, 0.0, 0.0, 1.0);
    meshSetVertex(mesh, 4, v);
    vecSet(8, v, right, bottom, lid, 1.0, 0.0, 0.0, 0.0, 1.0);
    meshSetVertex(mesh, 5, v);
    vecSet(8, v, right, top, lid, 1.0, 1.0, 0.0, 0.0, 1.0);
    meshSetVertex(mesh, 6, v);
    vecSet(8, v, left, top, lid, 0.0, 1.0, 0.0, 0.0, 1.0);
    meshSetVertex(mesh, 7, v);
    vecSet(8, v, left, top, base, 0.0, 1.0, 0.0, 1.0, 0.0);
    meshSetVertex(mesh, 8, v);
    vecSet(8, v, right, top, base, 1.0, 1.0, 0.0, 1.0, 0.0);
    meshSetVertex(mesh, 9, v);
    vecSet(8, v, right, top, lid, 1.0, 1.0, 0.0, 1.0, 0.0);
    meshSetVertex(mesh, 10, v);
    vecSet(8, v, left, top, lid, 0.0, 1.0, 0.0, 1.0, 0.0);
    meshSetVertex(mesh, 11, v);
    vecSet(8, v, left, bottom, base, 0.0, 0.0, 0.0, -1.0, 0.0);
    meshSetVertex(mesh, 12, v);
    vecSet(8, v, right, bottom, base, 1.0, 0.0, 0.0, -1.0, 0.0);
    meshSetVertex(mesh, 13, v);
    vecSet(8, v, right, bottom, lid, 1.0, 0.0, 0.0, -1.0, 0.0);
    meshSetVertex(mesh, 14, v);
    vecSet(8, v, left, bottom, lid, 0.0, 0.0, 0.0, -1.0, 0.0);
    meshSetVertex(mesh, 15, v);
    vecSet(8, v, right, top, base, 1.0, 1.0, 1.0, 0.0, 0.0);
    meshSetVertex(mesh, 16, v);
    vecSet(8, v, right, bottom, base, 1.0, 0.0, 1.0, 0.0, 0.0);
    meshSetVertex(mesh, 17, v);
    vecSet(8, v, right, bottom, lid, 1.0, 0.0, 1.0, 0.0, 0.0);
    meshSetVertex(mesh, 18, v);
    vecSet(8, v, right, top, lid, 1.0, 1.0, 1.0, 0.0, 0.0);
    meshSetVertex(mesh, 19, v);
    vecSet(8, v, left, top, base, 0.0, 1.0, -1.0, 0.0, 0.0);
    meshSetVertex(mesh, 20, v);
    vecSet(8, v, left, bottom, base, 0.0, 0.0, -1.0, 0.0, 0.0);
    meshSetVertex(mesh, 21, v);
    vecSet(8, v, left, bottom, lid, 0.0, 0.0, -1.0, 0.0, 0.0);
    meshSetVertex(mesh, 22, v);
    vecSet(8, v, left, top, lid, 0.0, 1.0, -1.0, 0.0, 0.0);
    meshSetVertex(mesh, 23, v);
    /* Now make vertex 0 for realsies. */
    vecSet(8, v, left, bottom, base, 0.0, 0.0, 0.0, 0.0, -1.0);
  }
  return error;
}

/* Rotates a 2-dimensional vector through an angle. The input can safely alias
the output. */
void meshRotateVector(GLdouble theta, GLdouble v[2], GLdouble vRot[2]) {
  GLdouble cosTheta = cos(theta);
  GLdouble sinTheta = sin(theta);
  GLdouble vRot0 = cosTheta * v[0] - sinTheta * v[1];
  vRot[1] = sinTheta * v[0] + cosTheta * v[1];
  vRot[0] = vRot0;
}

/* Rotate a curve about the Z-axis. Can be used to make a sphere, spheroid,
capsule, circular cone, circular cylinder, box, etc. The z-values should be in
ascending order --- or at least the first z should be less than the last. The
first and last r-values should be 0.0, and no others. Probably the t-values
should be in ascending or descending order. The sideNum parameter controls the
fineness of the mesh. The attributes are XYZ position, ST texture, and NOP unit
normal vector. The normals are smooth. Don't forget to meshDestroy when
finished. */
int meshInitializeRevolution(meshMesh *mesh, GLuint zNum, GLdouble z[],
                             GLdouble r[], GLdouble t[], GLuint sideNum) {
  GLuint i, j, error;
  error = meshInitialize(mesh, (zNum - 2) * sideNum * 2,
                         (zNum - 2) * (sideNum + 1) + 2, 3 + 2 + 3);
  if (error == 0) {
    /* Make the bottom triangles. */
    for (i = 0; i < sideNum; i += 1) meshSetTriangle(mesh, i, 0, i + 2, i + 1);
    /* Make the top triangles. */
    for (i = 0; i < sideNum; i += 1)
      meshSetTriangle(mesh, sideNum + i, mesh->vertNum - 1,
                      mesh->vertNum - 1 - (sideNum + 1) + i,
                      mesh->vertNum - 1 - (sideNum + 1) + i + 1);
    /* Make the middle triangles. */
    for (j = 1; j <= zNum - 3; j += 1)
      for (i = 0; i < sideNum; i += 1) {
        meshSetTriangle(
            mesh, 2 * sideNum * j + 2 * i, (j - 1) * (sideNum + 1) + 1 + i,
            j * (sideNum + 1) + 1 + i + 1, j * (sideNum + 1) + 1 + i);
        meshSetTriangle(
            mesh, 2 * sideNum * j + 2 * i + 1, (j - 1) * (sideNum + 1) + 1 + i,
            (j - 1) * (sideNum + 1) + 1 + i + 1, j * (sideNum + 1) + 1 + i + 1);
      }
    /* Make the vertices, using vertex 0 as temporary storage. */
    GLdouble *v = mesh->vert;
    GLdouble p[3], q[3], o[3];
    for (j = 1; j <= zNum - 2; j += 1) {
      // Form the sideNum + 1 vertices in the jth layer.
      vecSet(3, p, z[j + 1] - z[j], 0.0, r[j] - r[j + 1]);
      vecUnit(3, p, p);
      vecSet(3, q, z[j] - z[j - 1], 0.0, r[j - 1] - r[j]);
      vecUnit(3, q, q);
      vecAdd(3, p, q, o);
      vecUnit(3, o, o);
      vecSet(8, v, r[j], 0.0, z[j], 1.0, t[j], o[0], o[1], o[2]);
      meshSetVertex(mesh, j * (sideNum + 1), v);
      v[3] = 0.0;
      meshSetVertex(mesh, (j - 1) * (sideNum + 1) + 1, v);
      for (i = 1; i < sideNum; i += 1) {
        meshRotateVector(2 * M_PI / sideNum, v, v);
        v[3] += 1.0 / sideNum;
        meshRotateVector(2 * M_PI / sideNum, &v[5], &v[5]);
        meshSetVertex(mesh, (j - 1) * (sideNum + 1) + 1 + i, v);
      }
    }
    /* Form the top vertex. */
    vecSet(8, v, 0.0, 0.0, z[zNum - 1], 0.0, 0.0, 0.0, 0.0, 1.0);
    meshSetVertex(mesh, mesh->vertNum - 1, v);
    /* Finally form the bottom vertex. */
    vecSet(8, v, 0.0, 0.0, z[0], 0.0, 0.0, 0.0, 0.0, -1.0);
  }
  return error;
}

/* Builds a mesh for a sphere, centered at the origin, of radius r. The sideNum
and layerNum parameters control the fineness of the mesh. The attributes are
XYZ position, ST texture, and NOP unit normal vector. The normals are smooth.
Don't forget to meshDestroy when finished. */
int meshInitializeSphere(meshMesh *mesh, GLdouble r, GLuint layerNum,
                         GLuint sideNum) {
  GLuint error, i;
  GLdouble *ts = (GLdouble *)malloc((layerNum + 1) * 3 * sizeof(GLdouble));
  if (ts == NULL)
    return 1;
  else {
    GLdouble *zs = &ts[layerNum + 1];
    GLdouble *rs = &ts[2 * layerNum + 2];
    for (i = 0; i <= layerNum; i += 1) {
      ts[i] = (GLdouble)i / layerNum;
      zs[i] = -r * cos(ts[i] * M_PI);
      rs[i] = r * sin(ts[i] * M_PI);
    }
    error = meshInitializeRevolution(mesh, layerNum + 1, zs, rs, ts, sideNum);
    free(ts);
    return error;
  }
}

/* Builds a mesh for a circular cylinder with spherical caps, centered at the
origin, of radius r and length l > 2 * r. The sideNum and layerNum parameters
control the fineness of the mesh. The attributes are XYZ position, ST texture,
and NOP unit normal vector. The normals are smooth. Don't forget to meshDestroy
when finished. */
int meshInitializeCapsule(meshMesh *mesh, GLdouble r, GLdouble l,
                          GLuint layerNum, GLuint sideNum) {
  GLuint error, i;
  GLdouble theta;
  GLdouble *ts = (GLdouble *)malloc((6 * layerNum + 6) * sizeof(GLdouble));
  if (ts == NULL)
    return 1;
  else {
    GLdouble *zs = &ts[2 * layerNum + 2];
    GLdouble *rs = &ts[4 * layerNum + 4];
    zs[0] = -l / 2.0;
    rs[0] = 0.0;
    ts[0] = 0.0;
    for (i = 1; i <= layerNum; i += 1) {
      theta = M_PI / 2.0 * (3 + i / (GLdouble)layerNum);
      zs[i] = -l / 2.0 + r + r * sin(theta);
      rs[i] = r * cos(theta);
      ts[i] = (zs[i] + l / 2.0) / l;
    }
    for (i = 0; i < layerNum; i += 1) {
      theta = M_PI / 2.0 * i / (GLdouble)layerNum;
      zs[layerNum + 1 + i] = l / 2.0 - r + r * sin(theta);
      rs[layerNum + 1 + i] = r * cos(theta);
      ts[layerNum + 1 + i] = (zs[layerNum + 1 + i] + l / 2.0) / l;
    }
    zs[2 * layerNum + 1] = l / 2.0;
    rs[2 * layerNum + 1] = 0.0;
    ts[2 * layerNum + 1] = 1.0;
    error =
        meshInitializeRevolution(mesh, 2 * layerNum + 2, zs, rs, ts, sideNum);
    free(ts);
    return error;
  }
}

/* Builds a non-closed 'landscape' mesh based on a grid of Z-values. There are
width * height Z-values, which arrive in the data parameter. The mesh is made
of (width - 1) * (height - 1) squares, each made of two triangles. The spacing
parameter controls the spacing of the X- and Y-coordinates of the vertices. The
attributes are XYZ position, ST texture, and NOP unit normal vector. Don't
forget to call meshDestroy when finished with the mesh. To understand the exact
layout of the data, try this example code:
GLdouble zs[3][4] = {
        {10.0, 9.0, 7.0, 6.0},
        {6.0, 5.0, 3.0, 1.0},
        {4.0, 3.0, -1.0, -2.0}};
int error = meshInitializeLandscape(&mesh, 3, 4, 20.0, (GLdouble *)zs); */
int meshInitializeLandscape(meshMesh *mesh, GLuint width, GLuint height,
                            GLdouble spacing, GLdouble *data) {
  GLuint i, j, error;
  GLuint a, b, c, d;
  GLdouble *vert, diffSWNE, diffSENW;
  error = meshInitialize(mesh, 2 * (width - 1) * (height - 1), width * height,
                         3 + 2 + 3);
  if (error == 0) {
    /* Build the vertices with normals set to 0. */
    for (i = 0; i < width; i += 1)
      for (j = 0; j < height; j += 1) {
        vert = meshGetVertexPointer(mesh, i * height + j);
        vecSet(3 + 2 + 3, vert, i * spacing, j * spacing, data[i * height + j],
               (GLdouble)i, (GLdouble)j, 0.0, 0.0, 0.0);
      }
    /* Build the triangles. */
    for (i = 0; i < width - 1; i += 1)
      for (j = 0; j < height - 1; j += 1) {
        GLuint index = 2 * (i * (height - 1) + j);
        a = i * height + j;
        b = (i + 1) * height + j;
        c = (i + 1) * height + (j + 1);
        d = i * height + (j + 1);
        diffSWNE = fabs(meshGetVertexPointer(mesh, a)[2] -
                        meshGetVertexPointer(mesh, c)[2]);
        diffSENW = fabs(meshGetVertexPointer(mesh, b)[2] -
                        meshGetVertexPointer(mesh, d)[2]);
        if (diffSENW < diffSWNE) {
          meshSetTriangle(mesh, index, d, a, b);
          meshSetTriangle(mesh, index + 1, b, c, d);
        } else {
          meshSetTriangle(mesh, index, a, b, c);
          meshSetTriangle(mesh, index + 1, a, c, d);
        }
      }
    /* Set the normals. */
    meshSmoothNormals(mesh, 5);
  }
  return error;
}

/* Given a landscape, such as that built by meshInitializeLandscape. Builds a
new landscape mesh by extracting triangles based on how horizontal they are. If
noMoreThan is true, then triangles are kept that deviate from horizontal by no
more than angle. If noMoreThan is false, then triangles are kept that deviate
from horizontal by more than angle. Don't forget to call meshDestroy when
finished. Warning: May contain extraneous vertices not used by any triangle. */
int meshInitializeDissectedLandscape(meshMesh *mesh, meshMesh *land,
                                     GLdouble angle, GLuint noMoreThan) {
  GLuint error, i, j = 0, triNum = 0;
  GLuint *tri, *newTri;
  GLdouble normal[3];
  /* Count the triangles that are nearly horizontal. */
  for (i = 0; i < land->triNum; i += 1) {
    tri = meshGetTrianglePointer(land, i);
    meshTrueNormal(meshGetVertexPointer(land, tri[0]),
                   meshGetVertexPointer(land, tri[1]),
                   meshGetVertexPointer(land, tri[2]), normal);
    if ((noMoreThan && normal[2] >= cos(angle)) ||
        (!noMoreThan && normal[2] < cos(angle)))
      triNum += 1;
  }
  error = meshInitialize(mesh, triNum, land->vertNum, 3 + 2 + 3);
  if (error == 0) {
    /* Copy all of the vertices. */
    vecCopy(land->vertNum * (3 + 2 + 3), land->vert, mesh->vert);
    /* Copy just the horizontal triangles. */
    for (i = 0; i < land->triNum; i += 1) {
      tri = meshGetTrianglePointer(land, i);
      meshTrueNormal(meshGetVertexPointer(land, tri[0]),
                     meshGetVertexPointer(land, tri[1]),
                     meshGetVertexPointer(land, tri[2]), normal);
      if ((noMoreThan && normal[2] >= cos(angle)) ||
          (!noMoreThan && normal[2] < cos(angle))) {
        newTri = meshGetTrianglePointer(mesh, j);
        newTri[0] = tri[0];
        newTri[1] = tri[1];
        newTri[2] = tri[2];
        j += 1;
      }
    }
    /* Reset the normals, to make the cliff edges appear sharper. */
    meshSmoothNormals(mesh, 5);
  }
  return error;
}

int meshInitializeRainCloud(meshMesh *mesh, GLuint length, GLuint width,
                              GLuint height, GLuint density, GLuint attrDim) {

  GLuint error, i;
  GLdouble *ts = (GLdouble *)malloc(1 * 3 * sizeof(GLdouble));
  GLuint *verts = (GLuint*)malloc(3 * length * width * sizeof(GLuint));
  int runner = 0;
  GLuint addedVerts = 0;

  for (int i = 0; i < length; i+= density) {
    for (int j = density; j < width; j+= density) {
      verts[runner] = i;
      verts[runner + 1 ] = j;
      verts[runner + 2] = height;
      runner+= 3;
      addedVerts++;
    }
  }


  //meshPointInitialize(mesh, addedVerts, attrDim);
  meshInitialize(mesh, 3 * addedVerts, addedVerts, attrDim);


  int vertRun = 0;
  for(int i = 0; i < addedVerts; i++) {
    mesh->vert[vertRun] = verts[i*3];
    mesh->vert[vertRun + 1] = verts[(i*3) + 1];
    mesh->vert[vertRun + 2] = verts[(i*3) + 2];
    mesh->vert[vertRun + 3] = 0;
    mesh->vert[vertRun + 4] = 0;
    mesh->vert[vertRun + 5] = 0;
    mesh->vert[vertRun + 6] = 0;
    mesh->vert[vertRun + 7] = 0;
    vertRun += 8;
  }

  free(ts);
  free(verts);
  return error;
  }
