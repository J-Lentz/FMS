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
!> @defgroup horiz_interp_type_mod horiz_interp_type_mod
!> @ingroup horiz_interp
!> @brief define derived data type that contains indices and weights used for subsequent
!! interpolations.
!> @author Zhi Liang

!> @addtogroup
!> @{
module horiz_interp_type_mod

use mpp_mod, only : mpp_send, mpp_recv, mpp_sync_self, mpp_error, FATAL
use mpp_mod, only : mpp_pe, mpp_root_pe, mpp_npes
use mpp_mod, only : COMM_TAG_1, COMM_TAG_2

implicit none
private


! parameter to determine interpolation method
 integer, parameter :: CONSERVE = 1
 integer, parameter :: BILINEAR = 2
 integer, parameter :: SPHERICA = 3
 integer, parameter :: BICUBIC  = 4

public :: CONSERVE, BILINEAR, SPHERICA, BICUBIC
public :: horiz_interp_type, stats, assignment(=)

!> @}

!> @ingroup horiz_interp_type_mod
interface assignment(=)
  module procedure horiz_interp_type_eq
end interface

!<PUBLICTYPE >
!> @ingroup horiz_interp_type_mod
 type horiz_interp_type
   real,    dimension(:,:), allocatable   :: faci    !< weights for conservative scheme
   real,    dimension(:,:), allocatable   :: facj    !< weights for conservative scheme
   integer, dimension(:,:), allocatable   :: ilon    !< indices for conservative scheme
   integer, dimension(:,:), allocatable   :: jlat    !< indices for conservative scheme
   real,    dimension(:,:), allocatable   :: area_src               !< area of the source grid
   real,    dimension(:,:), allocatable   :: area_dst               !< area of the destination grid
   real,    dimension(:,:,:), allocatable :: wti       !< weights for bilinear interpolation
                                                           !! wti ist used for derivative "weights" in bicubic
   real,    dimension(:,:,:), allocatable :: wtj       !< weights for bilinear interpolation
                                                           !! wti ist used for derivative "weights" in bicubic
   integer, dimension(:,:,:), allocatable :: i_lon  !< indices for bilinear interpolation
                                                        !! and spherical regrid
   integer, dimension(:,:,:), allocatable :: j_lat  !< indices for bilinear interpolation
                                                        !! and spherical regrid
   real,    dimension(:,:,:), allocatable :: src_dist               !< distance between destination grid and
                                                                        !! neighbor source grid.
   logical, dimension(:,:), allocatable   :: found_neighbors       !< indicate whether destination grid
                                                                        !! has some source grid around it.
   real                               :: max_src_dist
   integer, dimension(:,:), allocatable   :: num_found
   integer                            :: nlon_src !< size of source grid
   integer                            :: nlat_src !< size of source grid
   integer                            :: nlon_dst !< size of destination grid
   integer                            :: nlat_dst !< size of destination grid
   integer                            :: interp_method      !< interpolation method.
                                                            !! =1, conservative scheme
                                                            !! =2, bilinear interpolation
                                                            !! =3, spherical regrid
                                                            !! =4, bicubic regrid
   real,    dimension(:,:), allocatable   :: rat_x  !< the ratio of coordinates of the dest grid
                                                        !! (x_dest -x_src_r)/(x_src_l -x_src_r)
                                                        !! and (y_dest -y_src_r)/(y_src_l -y_src_r)
   real,    dimension(:,:), allocatable   :: rat_y  !< the ratio of coordinates of the dest grid
                                                        !! (x_dest -x_src_r)/(x_src_l -x_src_r)
                                                        !! and (y_dest -y_src_r)/(y_src_l -y_src_r)
   real,    dimension(:), allocatable     :: lon_in   !< the coordinates of the source grid
   real,    dimension(:), allocatable     :: lat_in   !< the coordinates of the source grid
   logical                            :: I_am_initialized=.false.
   integer                            :: version                            !< indicate conservative
                                                                            !! interpolation version with value 1 or 2
   !--- The following are for conservative interpolation scheme version 2 ( through xgrid)
   integer                            :: nxgrid                             !< number of exchange grid
                                                                            !! between src and dst grid.
   integer, dimension(:), allocatable     :: i_src       !< indices in source grid.
   integer, dimension(:), allocatable     :: j_src       !< indices in source grid.
   integer, dimension(:), allocatable     :: i_dst       !< indices in destination grid.
   integer, dimension(:), allocatable     :: j_dst       !< indices in destination grid.
   real,    dimension(:), allocatable     :: area_frac_dst              !< area fraction in destination grid.
   real,    dimension(:,:), allocatable   :: mask_in
 end type
!</PUBLICTYPE>

!> @addtogroup horiz_interp_type_mod
!> @{
contains

!#######################################################################
 !> @brief This statistics is for bilinear interpolation and spherical regrid.
 subroutine stats ( dat, low, high, avg, miss, missing_value, mask )
 real,    intent(in)  :: dat(:,:)
 real,    intent(out) :: low, high, avg
 integer, intent(out) :: miss
 real, intent(in), optional :: missing_value
 real,    intent(in), optional :: mask(:,:)

 real :: dsum, buffer_real(3)
 integer :: pe, root_pe, npes, p, buffer_int(2), npts

   pe = mpp_pe()
   root_pe = mpp_root_pe()
   npes = mpp_npes()

   dsum = 0.0
   miss = 0

   if (present(missing_value)) then
      miss = count(dat(:,:) == missing_value)
      low  = minval(dat(:,:), dat(:,:) /= missing_value)
      high = maxval(dat(:,:), dat(:,:) /= missing_value)
      dsum = sum(dat(:,:), dat(:,:) /= missing_value)
   else if(present(mask)) then
      miss = count(mask(:,:) <= 0.5)
      low  = minval(dat(:,:),mask=mask(:,:) > 0.5)
      high = maxval(dat(:,:),mask=mask(:,:) > 0.5)
      dsum = sum(dat(:,:), mask=mask(:,:) > 0.5)
   else
      miss = 0
      low  = minval(dat(:,:))
      high = maxval(dat(:,:))
      dsum = sum(dat(:,:))
   endif
   avg = 0.0

   npts = size(dat(:,:)) - miss
   if(pe == root_pe) then
      do p = 1, npes - 1  ! root_pe receive data from other pe
      ! Force use of "scalar", integer pointer mpp interface
         call mpp_recv(buffer_real(1),glen=3, from_pe=p+root_pe, tag=COMM_TAG_1)
         dsum = dsum + buffer_real(1)
         low  = min(low, buffer_real(2))
         high = max(high, buffer_real(3))
         call mpp_recv(buffer_int(1), glen=2, from_pe=p+root_pe, tag=COMM_TAG_2)
         miss = miss + buffer_int(1)
         npts = npts + buffer_int(2)
      enddo
      if(npts == 0) then
         print*, 'Warning: no points is valid'
      else
         avg = dsum/real(npts)
      endif
    else   ! other pe send data to the root_pe.
      buffer_real(1) = dsum
      buffer_real(2) = low
      buffer_real(3) = high
      ! Force use of "scalar", integer pointer mpp interface
      call mpp_send(buffer_real(1),plen=3,to_pe=root_pe, tag=COMM_TAG_1)
      buffer_int(1) = miss
      buffer_int(2) = npts
      call mpp_send(buffer_int(1), plen=2, to_pe=root_pe, tag=COMM_TAG_2)
    endif

    call mpp_sync_self()

    return

 end subroutine stats

!######################################################################################################################
!> @brief horiz_interp_type_eq creates a copy of the horiz_interp_type object
 subroutine horiz_interp_type_eq(horiz_interp_out, horiz_interp_in)
    type(horiz_interp_type), intent(inout) :: horiz_interp_out !< Output object being set
    type(horiz_interp_type), intent(in)    :: horiz_interp_in !< Input object being copied

    if(.not.horiz_interp_in%I_am_initialized) then
      call mpp_error(FATAL,'horiz_interp_type_eq: horiz_interp_type variable on right hand side is unassigned')
    endif

    horiz_interp_out%faci            =  horiz_interp_in%faci
    horiz_interp_out%facj            =  horiz_interp_in%facj
    horiz_interp_out%ilon            =  horiz_interp_in%ilon
    horiz_interp_out%jlat            =  horiz_interp_in%jlat
    horiz_interp_out%area_src        =  horiz_interp_in%area_src
    horiz_interp_out%area_dst        =  horiz_interp_in%area_dst
    horiz_interp_out%wti             =  horiz_interp_in%wti
    horiz_interp_out%wtj             =  horiz_interp_in%wtj
    horiz_interp_out%i_lon           =  horiz_interp_in%i_lon
    horiz_interp_out%j_lat           =  horiz_interp_in%j_lat
    horiz_interp_out%src_dist        =  horiz_interp_in%src_dist
    if (allocated(horiz_interp_in%found_neighbors)) horiz_interp_out%found_neighbors = horiz_interp_in%found_neighbors
    horiz_interp_out%max_src_dist    =  horiz_interp_in%max_src_dist
    horiz_interp_out%num_found       =  horiz_interp_in%num_found
    horiz_interp_out%nlon_src        =  horiz_interp_in%nlon_src
    horiz_interp_out%nlat_src        =  horiz_interp_in%nlat_src
    horiz_interp_out%nlon_dst        =  horiz_interp_in%nlon_dst
    horiz_interp_out%nlat_dst        =  horiz_interp_in%nlat_dst
    horiz_interp_out%interp_method   =  horiz_interp_in%interp_method
    horiz_interp_out%rat_x           =  horiz_interp_in%rat_x
    horiz_interp_out%rat_y           =  horiz_interp_in%rat_y
    horiz_interp_out%lon_in          =  horiz_interp_in%lon_in
    horiz_interp_out%lat_in          =  horiz_interp_in%lat_in
    horiz_interp_out%I_am_initialized = .true.
    horiz_interp_out%i_src           =  horiz_interp_in%i_src
    horiz_interp_out%j_src           =  horiz_interp_in%j_src
    horiz_interp_out%i_dst           =  horiz_interp_in%i_dst
    horiz_interp_out%j_dst           =  horiz_interp_in%j_dst
    horiz_interp_out%area_frac_dst   =  horiz_interp_in%area_frac_dst
    if(horiz_interp_in%interp_method == CONSERVE) then
       horiz_interp_out%version =  horiz_interp_in%version
       if(horiz_interp_in%version==2) horiz_interp_out%nxgrid = horiz_interp_in%nxgrid
    end if

 end subroutine horiz_interp_type_eq
!######################################################################################################################

end module horiz_interp_type_mod
!> @}
! close documentation grouping