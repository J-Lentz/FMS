
!***********************************************************************
!*                   GNU Lesser General Public License
!*
!* This file is part of the GFDL Flexible Modeling System (FMS).
!*
!* FMS is free software: you can redistribute it and/or modify it under
!* the terms of the GNU Lesser General Public License as published by
!* the Free Software Foundation, either version 3 of the License, or (at
!* your option) any later version.
!*
!* FMS is distributed in the hope that it will be useful, but WITHOUT
!* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
!* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
!* for more details.
!*
!* You should have received a copy of the GNU Lesser General Public
!* License along with FMS.  If not, see <http://www.gnu.org/licenses/>.
!***********************************************************************
!> @defgroup axis_utils2_mod axis_utils2_mod
!> @ingroup axis_utils
!> @brief A set of utilities for manipulating axes and extracting axis attributes.
!! FMS2_IO equivalent version of @ref axis_utils_mod.
!> @author M.J. Harrison

!> @addtogroup axis_utils2_mod
!> @{

  !> get axis edge data from a given file
  subroutine AXIS_EDGES_(fileobj, name, edge_data, reproduce_null_char_bug_flag)

  class(FmsNetcdfFile_t), intent(in)            :: fileobj  !< File object to read from
  character(len=*), intent(in)                  :: name  !< Name of a given axis
  real(FMS_AU_KIND_), dimension(:), intent(out) :: edge_data  !< Returned edge data from given axis name
  logical, intent(in), optional                 :: reproduce_null_char_bug_flag  !< Flag indicating to reproduce
                                     !! the mpp_io bug where the null characters were not removed
                                     !! after reading a string attribute

  integer :: ndims
  character(len=128) :: buffer
  integer,                 dimension(:),   allocatable :: dim_sizes
  real(kind=FMS_AU_KIND_), dimension(:),   allocatable :: r_var
  real(kind=FMS_AU_KIND_), dimension(:,:), allocatable :: r2d
  integer :: i
  integer :: n
  logical :: reproduce_null_char_bug  !< Local flag indicating to reproduce the mpp_io bug where
                                     !! the null characters were not removed after reading a string attribute

  ndims = get_variable_num_dimensions(fileobj, name)
  allocate(dim_sizes(ndims))

  call get_variable_size(fileobj, name, dim_sizes)

  n = dim_sizes(1)
  if (size(edge_data) .ne. n+1) then
    call mpp_error(FATAL, "axis_edge: incorrect size of edge_data array.")
  endif
  deallocate(dim_sizes)

  reproduce_null_char_bug = .false.
  if (present(reproduce_null_char_bug_flag)) reproduce_null_char_bug = reproduce_null_char_bug_flag

  buffer = ""
  if (variable_att_exists(fileobj, name, "edges")) then
   !! If the reproduce_null_char_bug flag is turned on fms2io will not remove the null character
    call get_variable_attribute(fileobj, name, "edges", buffer(1:128), &
        reproduce_null_char_bug_flag=reproduce_null_char_bug)

   !! Check for a null character here, if it exists *_bnds will be calculated instead of read in
    if (reproduce_null_char_bug) then
        i = 0
        i = index(buffer, char(0))
        if (i > 0) buffer = ""
    endif
  elseif (variable_att_exists(fileobj, name, "bounds")) then
   !! If the reproduce_null_char_bug flag is turned on fms2io will not remove the null character
    call get_variable_attribute(fileobj, name, "bounds", buffer(1:128), &
        reproduce_null_char_bug_flag=reproduce_null_char_bug)

    !! Check for a null character here, if it exists *_bnds will be calculated instead of read in
    if (reproduce_null_char_bug) then
        i = 0
        i = index(buffer, char(0))
        if (i > 0) buffer = ""
    endif
  endif
  if (trim(buffer) .ne. "") then
    ndims = get_variable_num_dimensions(fileobj, buffer)
    allocate(dim_sizes(ndims))

    call get_variable_size(fileobj, buffer, dim_sizes)

    if (size(dim_sizes) .eq. 1) then
      if (dim_sizes(1) .ne. n+1) then
        call mpp_error(FATAL, "axis_edges: incorrect size of edge data.")
      endif

      call read_data(fileobj, buffer, edge_data)

    elseif (size(dim_sizes) .eq. 2) then
      if (dim_sizes(1) .ne. 2) then
        call mpp_error(FATAL, "axis_edges: first dimension of edge must be of size 2")
      endif
      if (dim_sizes(2) .ne. n) then
        call mpp_error(FATAL, "axis_edges: incorrect size of edge data.")
      endif

      allocate(r2d(dim_sizes(1), dim_sizes(2)))
      call read_data(fileobj, buffer, r2d)
      edge_data(1:dim_sizes(2)) = r2d(1,:)
      edge_data(dim_sizes(2)+1) = r2d(2,dim_sizes(2))
      deallocate(r2d)
    endif
    deallocate(dim_sizes)
  else
      allocate(r_var(n))

      call read_data(fileobj, name, r_var)

      do i = 2, n
         edge_data(i) = r_var(i-1) + real(0.5,kind=FMS_AU_KIND_)*(r_var(i) - r_var(i-1))
      enddo
      edge_data(1)    = r_var(1) - real(0.5,kind=FMS_AU_KIND_)*(r_var(2) - r_var(1))
      if (abs(edge_data(1)) .lt. real(1.e-10,kind=FMS_AU_KIND_)) then
         edge_data(1) = real(0.0,kind=FMS_AU_KIND_)
      endif
      edge_data(n+1)  = r_var(n) + real(0.5,kind=FMS_AU_KIND_)*(r_var(n) - r_var(n-1))
      deallocate(r_var)
  endif
end subroutine AXIS_EDGES_

  !> @brief Returns lon_strt <= longitude <= lon_strt+360
  !! @return real lon_in_range */

  function LON_IN_RANGE_(lon, l_strt)
    real(kind=FMS_AU_KIND_), intent(in) :: lon, l_strt
    real(kind=FMS_AU_KIND_)             :: LON_IN_RANGE_
    real(kind=FMS_AU_KIND_)             :: l_end

    LON_IN_RANGE_ = lon
    l_end = l_strt+real(360.0,kind=FMS_AU_KIND_)

    if (abs(LON_IN_RANGE_ - l_strt) < real(1.e-4,kind=FMS_AU_KIND_)) then
      LON_IN_RANGE_ = l_strt
      return
    endif

    if (abs(LON_IN_RANGE_ - l_end) < real(1.e-4,kind=FMS_AU_KIND_)) then
      LON_IN_RANGE_ = l_strt
      return
    endif

    do
      if (LON_IN_RANGE_ < l_strt) then
        LON_IN_RANGE_ = LON_IN_RANGE_ + f360
      else if (LON_IN_RANGE_  >  l_end) then
        LON_IN_RANGE_ = LON_IN_RANGE_ - f360
      else
        exit
      end if
    end do

  end function LON_IN_RANGE_

  !> @brief Returns monotonic array of longitudes s.t., lon_strt <= lon(:) <= lon_strt+360.
  !!
  !> <br>The first istrt-1 entries are moved to the end of the array:
  !!
  !! e.g.
  !!        lon =      0 1 2 3 4 5  ...  358 359; lon_strt = 3 ==>
  !!        tranlon =  3 4 5 6 7 8  ...  359 360 361 362; istrt = 4

  subroutine TRANLON_(lon, lon_start, istrt)

    ! returns array of longitudes s.t.  lon_strt <= lon < lon_strt+360.
    ! also, the first istrt-1 entries are moved to the end of the array
    !
    ! e.g.
    !        lon =      0 1 2 3 4 5  ...  358 359; lon_strt = 3 ==>
    !        tranlon =  3 4 5 6 7 8  ...  359 360 361 362; istrt = 4

    real(kind=FMS_AU_KIND_), intent(inout), dimension(:) :: lon
    real(kind=FMS_AU_KIND_), intent(in)                  :: lon_start
    integer, intent(out)                                 :: istrt


    integer                 :: len, i
    real(kind=FMS_AU_KIND_) :: lon_strt, tmp(size(lon(:))-1)

    len = size(lon(:))

    do i = 1, len
       lon(i) = LON_IN_RANGE_(lon(i),lon_start)
    enddo

    istrt = 0
    do i = 1,len-1
       if (lon(i+1) < lon(i)) then
          istrt = i+1
          exit
       endif
    enddo

    if (istrt>1) then ! grid is not monotonic
       if (abs(lon(len)-lon(1)) < epsln) then
          tmp = cshift(lon(1:len-1),istrt-1)
          lon(1:len-1) = tmp
          lon(len)     = lon(1)
       else
          lon = cshift(lon,istrt-1)
       endif

       lon_strt = lon(1)
       do i=2,len+1
          lon(i)   = LON_IN_RANGE_(lon(i),lon_strt)
          lon_strt = lon(i)
       enddo
    endif

    return
  end subroutine TRANLON_

  !>     nearest_index = index of nearest data point within "array" corresponding to
  !!            "value".
  !!
  !!     inputs:
  !!
  !!     value  = arbitrary data...same units as elements in "array"
  !!     array  = array of data points  (must be monotonically increasing)
  !!
  !!     output:
  !!
  !!     nearest_index =  index of nearest data point to "value"
  !!             if "value" is outside the domain of "array" then nearest_index = 1
  !!             or "ia" depending on whether array(1) or array(ia) is
  !!             closest to "value"
  !!
  !!             note: if "array" is dimensioned array(0:ia) in the calling
  !!                   program, then the returned index should be reduced
  !!                   by one to account for the zero base.
  !!
  !!     example:
  !!
  !!     let model depths be defined by the following:
  !!     parameter (km=5)
  !!     dimension z(km)
  !!     data z /5.0, 10.0, 50.0, 100.0, 250.0/
  !!
  !!     k1 = nearest_index (12.5, z, km)
  !!     k2 = nearest_index (0.0, z, km)
  !!
  !!     k1 would be set to 2, and k2 would be set to 1 so that
  !!     z(k1) would be the nearest data point to 12.5 and z(k2) would
  !!   be the nearest data point to 0.0
  !!
  !!   @return real frac_index


  function FRAC_INDEX_(value, array)

    integer                               :: ia, i, ii, unit
    real(kind=FMS_AU_KIND_)               :: value !< arbitrary data...same units as elements in "array"
    real(kind=FMS_AU_KIND_)               :: FRAC_INDEX_
    real(kind=FMS_AU_KIND_), dimension(:) :: array !< array of data points  (must be monotonically increasing)
    logical keep_going

    ia = size(array(:))

    do i = 2, ia
       if (array(i) < array(i-1)) then
          unit = stdout()
          write (unit,*) '=> Error: "frac_index" array must be monotonically' &
          & // 'increasing when searching for nearest value to ', value
          write (unit,*) '          array(i) < array(i-1) for i=',i
          write (unit,*) '          array(i) for i=1..ia follows:'
          do ii = 1, ia
             write (unit,*) 'i=',ii, ' array(i)=',array(ii)
          enddo
          call mpp_error(FATAL,' "frac_index" array must be monotonically increasing.')
       endif
    enddo

    if (value < array(1) .or. value > array(ia)) then
 !       if (value < array(1))  frac_index = 1.
 !       if (value > array(ia)) frac_index = float(ia)
        FRAC_INDEX_ = real(-1.0,kind=FMS_AU_KIND_)
    else
       i = 1
       keep_going = .true.
       do while (i <= ia .and. keep_going)
          i = i+1
          if (value <= array(i)) then
             FRAC_INDEX_ = float(i-1) + (value-array(i-1))/(array(i)-array(i-1))
             keep_going  = .false.
          endif
       enddo
    endif
  end function FRAC_INDEX_

  !> @brief Return index of nearest point along axis
  !!
  !>     nearest_index = index of nearest data point within "array" corresponding to
  !!            "value".
  !!
  !!     inputs:
  !!
  !!     value  = arbitrary data...same units as elements in "array"
  !!     array  = array of data points  (must be monotonically increasing)
  !!     ia     = dimension of "array"
  !!
  !!     output:
  !!
  !!     nearest_index =  index of nearest data point to "value"
  !!             if "value" is outside the domain of "array" then nearest_index = 1
  !!             or "ia" depending on whether array(1) or array(ia) is
  !!             closest to "value"
  !!
  !!             note: if "array" is dimensioned array(0:ia) in the calling
  !!                   program, then the returned index should be reduced
  !!                   by one to account for the zero base.
  !!
  !!     example:
  !!
  !!     let model depths be defined by the following:
  !!     parameter (km=5)
  !!     dimension z(km)
  !!     data z /5.0, 10.0, 50.0, 100.0, 250.0/
  !!
  !!     k1 = nearest_index (12.5, z, km)
  !!     k2 = nearest_index (0.0, z, km)
  !!
  !!     k1 would be set to 2, and k2 would be set to 1 so that
  !!     z(k1) would be the nearest data point to 12.5 and z(k2) would
  !!     be the nearest data point to 0.0
  !! @return integer nearest_index



  function NEAREST_INDEX_(value, array)

    integer :: NEAREST_INDEX_
    integer :: ia !< dimension of "array"
    integer :: i, ii, unit
    real(kind=FMS_AU_KIND_)               :: value !< arbitrary data...same units as elements in "array"
    real(kind=FMS_AU_KIND_), dimension(:) :: array !< array of data points  (must be monotonically increasing)
    logical keep_going

    ia = size(array(:))

    do i = 2, ia
       if (array(i) < array(i-1)) then
          unit = stdout()
          write (unit,*) '=> Error: "nearest_index" array must be monotonically increasing' &
                         & // 'when searching for nearest value to ', value
          write (unit,*) '          array(i) < array(i-1) for i=',i
          write (unit,*) '          array(i) for i=1..ia follows:'
          do ii = 1, ia
             write (unit,*) 'i=',ii, ' array(i)=', array(ii)
          enddo
          call mpp_error(FATAL,' "nearest_index" array must be monotonically increasing.')
       endif
    enddo

    if (value < array(1) .or. value > array(ia)) then
       if (value < array(1))  NEAREST_INDEX_ = 1
       if (value > array(ia)) NEAREST_INDEX_ = ia
    else
       i = 1
       keep_going = .true.
       do while (i <= ia .and. keep_going)
          i = i+1
          if (value <= array(i)) then
             NEAREST_INDEX_ = i
             if (array(i)-value > value-array(i-1)) NEAREST_INDEX_ = i-1
             keep_going = .false.
          endif
       enddo
    endif
  end function NEAREST_INDEX_

  !#############################################################################

  subroutine INTERP_1D_LINEAR_(grid1,grid2,data1,data2)

    real(kind=FMS_AU_KIND_), dimension(:),    intent(in) :: grid1, data1, grid2
    real(kind=FMS_AU_KIND_), dimension(:), intent(inout) :: data2

    integer :: n1, n2, i, n
    real(kind=FMS_AU_KIND_) :: w

    n1 = size(grid1(:))
    n2 = size(grid2(:))


    do i = 2, n1
       if (grid1(i) <= grid1(i-1)) call mpp_error(FATAL, 'grid1 not monotonic')
    enddo

    do i = 2, n2
       if (grid2(i) <= grid2(i-1)) call mpp_error(FATAL, 'grid2 not monotonic')
    enddo

    if (grid1(1) > grid2(1) ) call mpp_error(FATAL, 'grid2 lies outside grid1')
    if (grid1(n1) < grid2(n2) ) call mpp_error(FATAL, 'grid2 lies outside grid1')

    do i = 1, n2
       n = NEAREST_INDEX_(grid2(i),grid1)

       if (grid1(n) < grid2(i)) then
          w        = (grid2(i)-grid1(n))/(grid1(n+1)-grid1(n))
          data2(i) = (real(1.0,kind=FMS_AU_KIND_)-w)*data1(n) + w*data1(n+1)
       else
          if(n==1) then
             data2(i) = data1(n)
          else
             w        = (grid2(i)-grid1(n-1))/(grid1(n)-grid1(n-1))
             data2(i) = (real(1.0,kind=FMS_AU_KIND_)-w)*data1(n-1) + w*data1(n)
          endif
       endif
    enddo


    return

  end subroutine INTERP_1D_LINEAR_

  !###################################################################
  subroutine INTERP_1D_CUBIC_SPLINE_(grid1, grid2, data1, data2, yp1, ypn)

    real(kind=FMS_AU_KIND_), dimension(:),    intent(in)    :: grid1, grid2, data1
    real(kind=FMS_AU_KIND_), dimension(:),    intent(inout) :: data2
    real(kind=FMS_AU_KIND_),                  intent(in)    :: yp1, ypn

    real(kind=FMS_AU_KIND_), dimension(size(grid1))         :: y2, u
    real(kind=FMS_AU_KIND_)                                 :: sig, p, qn, un, h, a ,b
    integer                                                 :: n, m, i, k, klo, khi

    n = size(grid1(:))
    m = size(grid2(:))

    do i = 2, n
       if (grid1(i) <= grid1(i-1)) call mpp_error(FATAL, 'grid1 not monotonic')
    enddo

    do i = 2, m
       if (grid2(i) <= grid2(i-1)) call mpp_error(FATAL, 'grid2 not monotonic')
    enddo

    if (grid1(1) > grid2(1) ) call mpp_error(FATAL, 'grid2 lies outside grid1')
    if (grid1(n) < grid2(m) ) call mpp_error(FATAL, 'grid2 lies outside grid1')

    if (yp1>real(0.99e30, kind=FMS_AU_KIND_)) then
       y2(1) = real(0.0,kind=FMS_AU_KIND_)
       u(1)  = real(0.0,kind=FMS_AU_KIND_)
    else
       y2(1) = real(-0.5,kind=FMS_AU_KIND_)
       u(1)  = (real(3.0,kind=FMS_AU_KIND_))/(grid1(2)-grid1(1))*((data1(2)-data1(1))/(grid1(2)-grid1(1))-yp1)
    endif

    do i = 2, n-1
       sig   = (grid1(i)-grid1(i-1))/(grid1(i+1)-grid1(i-1))
       p     = sig*y2(i-1) + real(2.0,kind=FMS_AU_KIND_)
       y2(i) = (sig-real(1.0,kind=FMS_AU_KIND_))/p
       u(i)  = (real(6.0,kind=FMS_AU_KIND_)*((data1(i+1)-data1(i))/(grid1(i+1)-grid1(i))-(data1(i)-data1(i-1)) &
               /(grid1(i)-grid1(i-1)))/(grid1(i+1)-grid1(i-1))-sig*u(i-1))/p
    enddo

    if (ypn>real(.99e30,kind=FMS_AU_KIND_)) then
       qn = real(0.0,kind=FMS_AU_KIND_)
       un = real(0.0,kind=FMS_AU_KIND_)
    else
       qn = real(0.5,kind=FMS_AU_KIND_)
       un = (real(3.0,kind=FMS_AU_KIND_))/(grid1(n)-grid1(n-1))*(ypn-(data1(n)-data1(n-1))/(grid1(n)-grid1(n-1)))
    endif

    y2(n) = (un-qn*u(n-1))/(qn*y2(n-1)+real(1.0,kind=FMS_AU_KIND_))

    do  k = n-1,1,-1
       y2(k) = y2(k)*y2(k+1)+u(k)
    enddo

    do k = 1, m
       n = NEAREST_INDEX_(grid2(k),grid1)
       if (grid1(n) < grid2(k)) then
          klo = n
       else
          if(n==1) then
            klo = n
          else
            klo = n -1
          endif
       endif

       khi      = klo+1
       h        = grid1(khi)-grid1(klo)
       a        = (grid1(khi) - grid2(k))/h
       b        = (grid2(k) - grid1(klo))/h
       data2(k) = a*data1(klo) + b*data1(khi)+ ((a**3-a)*y2(klo) + (b**3-b)*y2(khi))*(h**2) &
                 /real(6.0,kind=FMS_AU_KIND_)
    enddo

  end subroutine INTERP_1D_CUBIC_SPLINE_

  !###################################################################

  subroutine INTERP_1D_1D_(grid1,grid2,data1,data2, method, yp1, yp2)

    real(kind=FMS_AU_KIND_), dimension(:),         intent(in)    :: grid1, data1, grid2
    real(kind=FMS_AU_KIND_), dimension(:),         intent(inout) :: data2
    character(len=*),                    optional, intent(in)    :: method
    real(kind=FMS_AU_KIND_),             optional, intent(in)    :: yp1, yp2

    real(kind=FMS_AU_KIND_)                                      :: y1, y2
    character(len=32)                                            :: interp_method
    integer                                                      :: k2, ks, ke

    k2 = size(grid2(:))

    interp_method = "linear"
    if(present(method)) interp_method = method
    y1 = real(1.0e30, kind=FMS_AU_KIND_)

    if(present(yp1)) y1 = yp1
    y2 = real(1.0e30, kind=FMS_AU_KIND_)

    if(present(yp2)) y2 = yp2
    call FIND_INDEX_(grid1, grid2(1), grid2(k2), ks, ke)
    select case(trim(interp_method))
    case("linear")
       call INTERP_1D_LINEAR_(grid1(ks:ke),grid2,data1(ks:ke),data2)
    case("cubic_spline")
       call INTERP_1D_CUBIC_SPLINE_(grid1(ks:ke),grid2,data1(ks:ke),data2, y1, y2)
    case default
       call mpp_error(FATAL,"axis_utils: interp_method should be linear or cubic_spline")
    end select

    return

  end subroutine INTERP_1D_1D_

 !###################################################################


  subroutine INTERP_1D_2D_(grid1,grid2,data1,data2)

    real(kind=FMS_AU_KIND_), dimension(:,:),    intent(in) :: grid1, data1, grid2
    real(kind=FMS_AU_KIND_), dimension(:,:), intent(inout) :: data2

    integer :: n1, n2, n, k2, ks, ke

    n1 = size(grid1,1)
    n2 = size(grid2,1)
    k2 = size(grid2,2)

    if (n1 /= n2) call mpp_error(FATAL,'grid size mismatch')

    do n = 1, n1
       call FIND_INDEX_(grid1(n,:), grid2(n,1), grid2(n,k2), ks, ke)
       call INTERP_1D_LINEAR_(grid1(n,ks:ke),grid2(n,:),data1(n,ks:ke),data2(n,:))
    enddo

    return

  end subroutine INTERP_1D_2D_

  !###################################################################

  subroutine INTERP_1D_3D_(grid1,grid2,data1,data2, method, yp1, yp2)

    real(FMS_AU_KIND_), dimension(:,:,:), intent(in)    :: grid1, data1, grid2
    real(FMS_AU_KIND_), dimension(:,:,:), intent(inout) :: data2
    character(len=*),           optional, intent(in)    :: method
    real(kind=FMS_AU_KIND_),    optional, intent(in)    :: yp1, yp2

    integer                                             :: n1, n2, m1, m2, k2, n, m
    real(kind=FMS_AU_KIND_)                             :: y1, y2
    character(len=32)                                   :: interp_method
    integer                                             :: ks, ke

    n1 = size(grid1,1)
    n2 = size(grid2,1)
    m1 = size(grid1,2)
    m2 = size(grid2,2)
    k2 = size(grid2,3)

    interp_method = "linear"
    if(present(method)) interp_method = method
    y1 = real(1.0e30, kind=FMS_AU_KIND_)

    if(present(yp1)) y1 = yp1
    y2 = real(1.0e30, kind=FMS_AU_KIND_)
    if(present(yp2)) y2 = yp2

    if (n1 /= n2 .or. m1 /= m2) call mpp_error(FATAL,'grid size mismatch')

    select case(trim(interp_method))
    case("linear")
       do m = 1, m1
          do n = 1, n1
            call FIND_INDEX_(grid1(n,m,:), grid2(n,m,1), grid2(n,m,k2), ks, ke)
            call INTERP_1D_LINEAR_(grid1(n,m,ks:ke),grid2(n,m,:),data1(n,m,ks:ke),data2(n,m,:))
          enddo
       enddo

    case("cubic_spline")
       do m = 1, m1
          do n = 1, n1
            call FIND_INDEX_(grid1(n,m,:), grid2(n,m,1), grid2(n,m,k2), ks, ke)
            call INTERP_1D_CUBIC_SPLINE_(grid1(n,m,ks:ke),grid2(n,m,:), data1(n,m,ks:ke),data2(n,m,:), y1, y2)
          enddo
       enddo

    case default
       call mpp_error(FATAL,"axis_utils: interp_method should be linear or cubic_spline")
    end select

    return

  end subroutine INTERP_1D_3D_


  !#####################################################################
  subroutine FIND_INDEX_(grid1, xs, xe, ks, ke)
    real(kind=FMS_AU_KIND_), dimension(:), intent(in)  :: grid1
    real(kind=FMS_AU_KIND_),               intent(in)  :: xs, xe
    integer,                               intent(out) :: ks, ke

    integer :: k, nk

    nk = size(grid1(:))

    ks = 0; ke = 0
    do k = 1, nk-1
       if(grid1(k) <= xs .and. grid1(k+1) > xs ) then
          ks = k
          exit
       endif
    enddo

    do k = nk, 2, -1
       if(grid1(k) >= xe .and. grid1(k-1) < xe ) then
          ke = k
          exit
       endif
    enddo

    if(ks == 0 ) call mpp_error(FATAL,' xs locate outside of grid1')
    if(ke == 0 ) call mpp_error(FATAL,' xe locate outside of grid1')

  end subroutine FIND_INDEX_
 !> @}
 ! close documentation grouping