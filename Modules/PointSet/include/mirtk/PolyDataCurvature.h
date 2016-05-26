/*
 * Medical Image Registration ToolKit (MIRTK)
 *
 * Copyright 2013-2015 Imperial College London
 * Copyright 2013-2015 Andreas Schuh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MIRTK_PolyDataCurvature_H
#define MIRTK_PolyDataCurvature_H

#include "mirtk/PolyDataFilter.h"

#include "mirtk/PointSetExport.h"

#include "vtkDataArray.h"


namespace mirtk {


/**
 * Compute curvature at each point of a surface mesh
 *
 * This curvature computation is based on a robust estimation of the 3D curvature
 * tensor field as implemented in Matlab by Gabriel Peyre:
 *
 * http://uk.mathworks.com/matlabcentral/fileexchange/5355-toolbox-graph/content/toolbox_graph/compute_curvature.m
 *
 * The algorithm is detailed in
 *       David Cohen-Steiner and Jean-Marie Morvan.
 *       Restricted Delaunay triangulations and normal cycle.
 *       In Proc. 19th Annual ACM Symposium on Computational Geometry,
 *       pages 237-246, 2003.
 *   and also in
 *       Pierre Alliez, David Cohen-Steiner, Olivier Devillers, Bruno Levy, and Mathieu Desbrun.
 *       Anisotropic Polygonal Remeshing.
 *       ACM Transactions on Graphics, 2003.
 *       Note: SIGGRAPH '2003 Conference Proceedings
 *
 * Alternatively, the vtkCurvatures filter can be used to compute mean curvature,
 * Gauss curvature, and minimum and maximum curvature. To enable the use of this
 * VTK filter, call VtkCurvaturesOn(). This option is ignored when the curvature
 * tensor or principle directions are requested. If multiple curvature types
 * supported by vtkCurvatures are requested, the minimum and maximum curvatures
 * are computed by this filter from the mean and Gauss curvature values obtained
 * by vtkCurvatures, to avoid the duplicate computation of these curvature
 * values when using the vtkCurvatures filter. This computation is identical to
 * vtkCurvatures::GetMinimumCurvature and vtkCurvatures::GetMaximumCurvature.
 *
 * @attention vtkPolyData::BuildLinks has to be called before filter execution.
 */
class PolyDataCurvature : public PolyDataFilter
{
  mirtkObjectMacro(PolyDataCurvature);

  // ---------------------------------------------------------------------------
  // Types

public:

  /// Enumeration of output curvature measures, can be combined using bitwise OR
  enum Type
  {
    Minimum          =   1, ///< Minimum principle curvature, k_min
    Maximum          =   2, ///< Maximum principle curvature, k_max
    Mean             =   4, ///< Mean curvature, (k_min + k_max)/2
    Gauss            =   8, ///< Gauss curvature, k_min * k_max
    Curvedness       =  16, ///< Curvedness, i.e., sqrt((k_min^2 + k_max^2) / 2)
    Normal           =  32, ///< Point normal computed from curvature tensor
    MinimumDirection =  64, ///< Direction of minimum curvature
    MaximumDirection = 128, ///< Direction of maximum curvature
    Tensor           = 256, ///< 6 entries of symmetric 3x3 curvature tensor
    InverseTensor    = 512, ///< 6 entries of inverse of symmetric 3x3 curvature tensor
    Scalars          = Minimum | Maximum | Mean | Gauss | Curvedness,
    Directions       = Normal | MinimumDirection | MaximumDirection
  };

  /// Component indices of tensor output array
  ///
  /// This order is compatible with ParaView, which automatically labels the
  /// entries in this order.
  enum TensorIndex
  {
    XX = 0, YY = 1, ZZ = 2, XY = 3, YX = XY, YZ = 4, ZY = YZ, XZ = 5, ZX = XZ
  };

  MIRTK_PointSet_EXPORT static const char *MINIMUM;           ///< Name of minimum curvature array
  MIRTK_PointSet_EXPORT static const char *MAXIMUM;           ///< Name of maximum curvature array
  MIRTK_PointSet_EXPORT static const char *MEAN;              ///< Name of mean curvature array
  MIRTK_PointSet_EXPORT static const char *GAUSS;             ///< Name of Gauss curvature array
  MIRTK_PointSet_EXPORT static const char *CURVEDNESS;        ///< Name of curvedness array
  MIRTK_PointSet_EXPORT static const char *NORMALS;           ///< Name of curvature-based normals array
  MIRTK_PointSet_EXPORT static const char *MINIMUM_DIRECTION; ///< Name of minimum curvature direction array
  MIRTK_PointSet_EXPORT static const char *MAXIMUM_DIRECTION; ///< Name of maximum curvature direction array
  MIRTK_PointSet_EXPORT static const char *TENSOR;            ///< Name of curvature tensor array
  MIRTK_PointSet_EXPORT static const char *INVERSE_TENSOR;    ///< Name of inverse curvature tensor array

  // ---------------------------------------------------------------------------
  // Attributes

protected:

  /// Curvature type(s) to compute
  mirtkPublicAttributeMacro(int, CurvatureType);

  /// Whether to use vtkCurvatures filter instead
  mirtkPublicAttributeMacro(bool, VtkCurvatures);

  /// Number of tensor averaging iterations
  mirtkPublicAttributeMacro(int, TensorAveraging);

  /// Whether to normalize curvature assuming a spherical shape
  mirtkPublicAttributeMacro(bool, Normalize);

  /// Volume of convex hull
  /// \note Compute only if NormalizeOn.
  mirtkReadOnlyAttributeMacro(double, Volume);

  /// Radius of sphere have the same volume as the convex hull
  /// \note Compute only if NormalizeOn.
  mirtkReadOnlyAttributeMacro(double, Radius);

  /// Copy attributes of this class from another instance
  void CopyAttributes(const PolyDataCurvature &);

public:

  // ---------------------------------------------------------------------------
  // Construction/Destruction

  /// Default constructor
  PolyDataCurvature(int type = Scalars);

  /// Copy constructor
  PolyDataCurvature(const PolyDataCurvature &);

  /// Assignment operator
  PolyDataCurvature &operator =(const PolyDataCurvature &);

  /// Destructor
  virtual ~PolyDataCurvature();

  // ---------------------------------------------------------------------------
  // Execution

  /// Initialize filter after input and parameters are set
  virtual void Initialize();

  /// Get/compute minimum curvature
  ///
  /// If Run was executed and the minimum curvature was included in the
  /// requested output curvature types, this function just returns the
  /// respective point data array. Otherwise, Initialize must be called
  /// first and this function will compute the mean, Gauss, and minimum
  /// curvatures using the vtkCurvatures filter.
  ///
  /// \note Smoothing is not performed unless Run is executed.
  vtkDataArray *GetMinimumCurvature();

  /// Get/compute maximum curvature
  ///
  /// If Run was executed and the maximum curvature was included in the
  /// requested output curvature types, this function just returns the
  /// respective point data array. Otherwise, Initialize must be called
  /// first and this function will compute the mean, Gauss, and maximum
  /// curvatures using the vtkCurvatures filter.
  ///
  /// \note Smoothing is not performed unless Run is executed.
  vtkDataArray *GetMaximumCurvature();

  /// Get/compute mean curvature
  ///
  /// If Run was executed and the mean curvature was included in the
  /// requested output curvature types, this function just returns the
  /// respective point data array. Otherwise, Initialize must be called
  /// first and this function will compute the mean curvature only using
  /// the vtkCurvatures filter.
  ///
  /// \note Smoothing is not performed unless Run is executed.
  vtkDataArray *GetMeanCurvature();

  /// Get/compute Gauss curvature
  ///
  /// If Run was executed and the Gauss curvature was included in the
  /// requested output curvature types, this function just returns the
  /// respective point data array. Otherwise, Initialize must be called
  /// first and this function will compute the Gauss curvature only using
  /// the vtkCurvatures filter.
  ///
  /// \note Smoothing is not performed unless Run is executed.
  vtkDataArray *GetGaussCurvature();

  /// Get/compute curvedness
  ///
  /// If Run was executed and the curvedness was included in the
  /// requested output curvature types, this function just returns the
  /// respective point data array. Otherwise, Initialize must be called
  /// first and this function will compute the mean, Gauss, minimum, and
  /// maximum curvature using the vtkCurvatures filter and then compute the
  /// curvedness from the minimum and maximum curvature.
  ///
  /// \note Smoothing is not performed unless Run is executed.
  vtkDataArray *GetCurvedness();

protected:

  /// Execute filter
  virtual void Execute();

  /// Compute curvature tensors
  void ComputeTensorField();

  /// Compute eigenvalues (and eigenvectors) of curvature tensors
  void DecomposeTensorField();

  /// Compute mean curvature
  void ComputeMeanCurvature();

  /// Compute Gauss curvature
  void ComputeGaussCurvature();

  /// Compute minimum and/or maximum curvature from mean and Gauss curvature
  void ComputeMinMaxCurvature(bool, bool);

  /// Compute curvedness measure
  void ComputeCurvedness();

  /// Finalize filter execution
  virtual void Finalize();

  // ---------------------------------------------------------------------------
  // Alternative VTK-like interface

public:

  /// Set output curvature types
  mirtkSetMacro(CurvatureType, int);

  /// Get output curvature types
  mirtkGetMacro(CurvatureType, int);

  /// Enable/disable use of vtkCurvatures filter
  mirtkOnOffMacro(VtkCurvatures);

  /// Enable/disable normalization of mean and Gauss curvature
  mirtkOnOffMacro(Normalize);

  /// Set number of curvature tensor averaging iterations
  mirtkSetMacro(TensorAveraging, int);

  /// Get number of curvature tensor averaging iterations
  mirtkGetMacro(TensorAveraging, int);

};


} // namespace mirtk

#endif // MIRTK_PolyDataCurvature_H
