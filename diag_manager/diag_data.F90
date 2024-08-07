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
!> @defgroup diag_data_mod diag_data_mod
!> @ingroup diag_manager
!! @brief Type descriptions and global variables for the diag_manager modules.
!! @author Seth Underwood <seth.underwood@noaa.gov>
!!
!!   Notation:
!!   <DL>
!!     <DT>input field</DT>
!!     <DD>The data structure describing the field as
!!       registered by the model code.</DD>
!!
!!     <DT>output field</DT>
!!     <DD>The data structure describing the actual
!!       diagnostic output with requested frequency and
!!       other options.</DD>
!!   </DL>
!!
!!   Input fields, output fields, and output files are gathered in arrays called
!!   "input_fields", "output_fields", and "files", respectively. Indices in these
!!   arrays are used as pointers to create associations between various data
!!   structures.
!!
!!   Each input field associated with one or several output fields via array of
!!   indices output_fields; each output field points to the single "parent" input
!!   field with the input_field index, and to the output file with the output_file
!!   index.

!> @addtogroup diag_data_mod
!> @{
MODULE diag_data_mod
use platform_mod

  USE time_manager_mod, ONLY: get_calendar_type, NO_CALENDAR, set_date, set_time, month_name, time_type
  USE constants_mod, ONLY: SECONDS_PER_HOUR, SECONDS_PER_MINUTE
  USE mpp_domains_mod, ONLY: domain1d, domain2d, domainUG
  USE fms_mod, ONLY: write_version_number
  USE fms_diag_bbox_mod, ONLY: fmsDiagIbounds_type
  use mpp_mod, ONLY: mpp_error, FATAL, WARNING, mpp_pe, mpp_root_pe, stdlog

  ! NF90_FILL_REAL has value of 9.9692099683868690e+36.
  USE netcdf, ONLY: NF_FILL_REAL => NF90_FILL_REAL
  use fms2_io_mod

  IMPLICIT NONE

  PUBLIC

  ! Specify storage limits for fixed size tables used for pointers, etc.
  integer, parameter :: diag_null = -999 !< Integer represening NULL in the diag_object
  character(len=1), parameter :: diag_null_string = " "
  integer, parameter :: diag_not_found = -1
  integer, parameter :: diag_not_registered = 0
  integer, parameter :: diag_registered_id = 10
  !> Supported averaging intervals
  integer, parameter :: monthly = 30
  integer, parameter :: daily = 24
  integer, parameter :: diurnal = 2
  integer, parameter :: yearly = 12
  integer, parameter :: no_diag_averaging = 0
  integer, parameter :: instantaneous = 0
  integer, parameter :: three_hourly = 3
  integer, parameter :: six_hourly = 6
  !integer, parameter :: seasonally = 180
  !> Supported type/kind of the variable
  !integer, parameter :: r16=16
  integer, parameter :: r8 = 8
  integer, parameter :: r4 = 4
  integer, parameter :: i8 = -8
  integer, parameter :: i4 = -4
  integer, parameter :: string = 19 !< s is the 19th letter of the alphabet
  integer, parameter :: null_type_int = -999
  INTEGER, PARAMETER :: MAX_FIELDS_PER_FILE = 300 !< Maximum number of fields per file.
  INTEGER, PARAMETER :: DIAG_OTHER = 0
  INTEGER, PARAMETER :: DIAG_OCEAN = 1
  INTEGER, PARAMETER :: DIAG_ALL   = 2
  INTEGER, PARAMETER :: VERY_LARGE_FILE_FREQ = 100000
  INTEGER, PARAMETER :: VERY_LARGE_AXIS_LENGTH = 10000
  INTEGER, PARAMETER :: EVERY_TIME =  0
  INTEGER, PARAMETER :: END_OF_RUN = -1
  INTEGER, PARAMETER :: DIAG_SECONDS = 1, DIAG_MINUTES = 2, DIAG_HOURS = 3
  INTEGER, PARAMETER :: DIAG_DAYS = 4, DIAG_MONTHS = 5, DIAG_YEARS = 6
  INTEGER, PARAMETER :: MAX_SUBAXES = 10
  INTEGER, PARAMETER :: NO_DOMAIN    = 1 !< Use the FmsNetcdfFile_t fileobj
  INTEGER, PARAMETER :: TWO_D_DOMAIN = 2 !< Use the FmsNetcdfDomainFile_t fileobj
  INTEGER, PARAMETER :: UG_DOMAIN    = 3 !< Use the FmsNetcdfUnstructuredDomainFile_t fileobj
  INTEGER, PARAMETER :: SUB_REGIONAL = 4 !< This is a file with a sub_region use the FmsNetcdfFile_t fileobj
  INTEGER, PARAMETER :: DIRECTION_UP   = 1  !< The axis points up if positive
  INTEGER, PARAMETER :: DIRECTION_DOWN = -1 !< The axis points down if positive
  INTEGER, PARAMETER :: GLO_REG_VAL = -999 !< Value used in the region specification of the diag_table
                                           !! to indicate to use the full axis instead of a sub-axis
  INTEGER, PARAMETER :: GLO_REG_VAL_ALT = -1 !< Alternate value used in the region specification of the
                                             !! diag_table to indicate to use the full axis instead of a sub-axis
  REAL(r8_kind), PARAMETER :: CMOR_MISSING_VALUE = 1.0e20 !< CMOR standard missing value
  INTEGER, PARAMETER :: DIAG_FIELD_NOT_FOUND = -1 !< Return value for a diag_field that isn't found in the diag_table
  INTEGER, PARAMETER :: latlon_gridtype = 1
  INTEGER, PARAMETER :: index_gridtype = 2
  INTEGER, PARAMETER :: null_gridtype = DIAG_NULL
  INTEGER, PARAMETER :: time_none    = 0 !< There is no reduction method
  INTEGER, PARAMETER :: time_min     = 1 !< The reduction method is min value
  INTEGER, PARAMETER :: time_max     = 2 !< The reduction method is max value
  INTEGER, PARAMETER :: time_sum     = 3 !< The reduction method is sum of values
  INTEGER, PARAMETER :: time_average= 4 !< The reduction method is average of values
  INTEGER, PARAMETER :: time_rms     = 5 !< The reudction method is root mean square of values
  INTEGER, PARAMETER :: time_diurnal = 6 !< The reduction method is diurnal
  INTEGER, PARAMETER :: time_power   = 7 !< The reduction method is average with exponents
  CHARACTER(len=7)   :: avg_name = 'average' !< Name of the average fields
  CHARACTER(len=8)   :: no_units = "NO UNITS"!< String indicating that the variable has no units
  INTEGER, PARAMETER :: begin_time  = 1 !< Use the begining of the time average bounds
  INTEGER, PARAMETER :: middle_time = 2 !< Use the middle of the time average bounds
  INTEGER, PARAMETER :: end_time    = 3 !< Use the end of the time average bounds
  INTEGER, PARAMETER :: MAX_STR_LEN = 255 !< Max length for a string
  INTEGER, PARAMETER :: is_x_axis = 1 !< integer indicating that it is a x axis
  INTEGER, PARAMETER :: is_y_axis = 2 !< integer indicating that it is a y axis
  !> @}

  !> @brief Contains the coordinates of the local domain to output.
  !> @ingroup diag_data_mod
  TYPE diag_grid
     REAL, DIMENSION(3) :: start !< start coordinates (lat,lon,depth) of local domain to output
     REAL, DIMENSION(3) :: END !< end coordinates (lat,lon,depth) of local domain to output
     INTEGER, DIMENSION(3) :: l_start_indx !< start indices at each LOCAL PE
     INTEGER, DIMENSION(3) :: l_end_indx !< end indices at each LOCAL PE
     INTEGER, DIMENSION(3) :: subaxes !< id returned from diag_subaxes_init of 3 subaxes
  END TYPE diag_grid

  !> @brief Diagnostic field type
  !> @ingroup diag_data_mod
  TYPE diag_fieldtype
     TYPE(domain2d) :: Domain
     TYPE(domainUG) :: DomainU
     REAL :: miss, miss_pack
     LOGICAL :: miss_present, miss_pack_present
     INTEGER :: tile_count
     character(len=128)      :: fieldname !< Fieldname
  END TYPE diag_fieldtype

  !> @brief Attribute type for diagnostic fields
  !> @ingroup diag_data_mod
  TYPE :: diag_atttype
     INTEGER             :: type !< Data type of attribute values (NF_INT, NF_FLOAT, NF_CHAR)
     INTEGER             :: len !< Number of values in attribute, or if a character string then
                                !! length of the string.
     CHARACTER(len=128)  :: name !< Name of the attribute
     CHARACTER(len=1280) :: catt !< Character string to hold character value of attribute
     REAL, allocatable, DIMENSION(:)    :: fatt !< REAL array to hold value of REAL attributes
     INTEGER, allocatable, DIMENSION(:) :: iatt !< INTEGER array to hold value of INTEGER attributes
  END TYPE diag_atttype

  !!TODO: coord_type deserves a better name, like coord_interval_type or coord_bbox_type.
  !!  additionally, consider using a 2D array.
  !> @brief Define the region for field output
  !> @ingroup diag_data_mod
  TYPE coord_type
     REAL :: xbegin
     REAL :: xend
     REAL :: ybegin
     REAL :: yend
     REAL :: zbegin
     REAL :: zend
  END TYPE coord_type

  !> @brief Type to define the diagnostic files that will be written as defined by the diagnostic table.
  !> @ingroup diag_data_mod
  TYPE file_type
     CHARACTER(len=128) :: name !< Name of the output file.
     CHARACTER(len=128) :: long_name
     INTEGER, DIMENSION(max_fields_per_file) :: fields
     INTEGER :: num_fields
     INTEGER :: output_freq
     INTEGER :: output_units
     INTEGER :: FORMAT
     INTEGER :: time_units
     INTEGER :: file_unit
     INTEGER :: bytes_written
     INTEGER :: time_axis_id, time_bounds_id
     INTEGER :: new_file_freq !< frequency to create new file
     INTEGER :: new_file_freq_units !< time units of new_file_freq (days, hours, years, ...)
     INTEGER :: duration
     INTEGER :: duration_units
     INTEGER :: tile_count
     LOGICAL :: local !< .TRUE. if fields are output in a region instead of global.
     TYPE(time_type) :: last_flush
     TYPE(time_type) :: next_open !< Time to open a new file.
     TYPE(time_type) :: start_time !< Time file opened.
     TYPE(time_type) :: close_time !< Time file closed.  File does not allow data after close time
     TYPE(diag_fieldtype):: f_avg_start, f_avg_end, f_avg_nitems, f_bounds
     TYPE(diag_atttype), allocatable, dimension(:) :: attributes !< Array to hold user definable attributes
     INTEGER :: num_attributes !< Number of defined attibutes
!----------
!ug support
     logical(I4_KIND) :: use_domainUG = .false.
     logical(I4_KIND) :: use_domain2D = .false.
!----------
!Check if time axis was already registered
     logical, allocatable :: is_time_axis_registered
!Support for fms2_io time
     real :: rtime_current
     integer :: time_index
     CHARACTER(len=10) :: filename_time_bounds
  END TYPE file_type

  !> @brief Type to hold the input field description
  !> @ingroup diag_data_mod
  TYPE input_field_type
     CHARACTER(len=128) :: module_name, field_name, long_name, units
     CHARACTER(len=256) :: standard_name
     CHARACTER(len=64) :: interp_method
     INTEGER, DIMENSION(3) :: axes
     INTEGER :: num_axes
     LOGICAL :: missing_value_present, range_present
     REAL :: missing_value
     REAL, DIMENSION(2) :: range
     INTEGER, allocatable, dimension(:) :: output_fields
     INTEGER :: num_output_fields
     INTEGER, DIMENSION(3) :: size
     LOGICAL :: static, register, mask_variant, local
     INTEGER :: numthreads
     INTEGER :: active_omp_level !< The current level of OpenMP nesting
     INTEGER :: tile_count
     TYPE(coord_type) :: local_coord
     TYPE(time_type)  :: time
     LOGICAL :: issued_mask_ignore_warning !< Indicates if the mask_ignore_warning
                                           !! has been issued for this input
                                           !! field.  Once .TRUE. the warning message
                                           !! is suppressed on all subsequent
                                           !! send_data calls.
  END TYPE input_field_type

  !> @brief Type to hold the output field description.
  !> @ingroup diag_data_mod
  TYPE output_field_type
     INTEGER :: input_field !< index of the corresponding input field in the table
     INTEGER :: output_file !< index of the output file in the table
     CHARACTER(len=128) :: output_name
     LOGICAL :: time_average !< true if the output field is averaged over time interval
     LOGICAL :: time_rms !< true if the output field is the rms.  If true, then time_average is also
     LOGICAL :: static
     LOGICAL :: time_max !< true if the output field is maximum over time interval
     LOGICAL :: time_min !< true if the output field is minimum over time interval
     LOGICAL :: time_sum !< true if the output field is summed over time interval
     LOGICAL :: time_ops !< true if any of time_min, time_max, time_rms or time_average is true
     INTEGER  :: pack
     INTEGER :: pow_value !< Power value to use for mean_pow(n) calculations
     CHARACTER(len=50) :: time_method !< time method field from the input file
     ! coordinates of the buffer and counter are (x, y, z, time-of-day)
     REAL, allocatable, DIMENSION(:,:,:,:) :: buffer !< coordinates of the buffer and counter are (x,
                                                     !! y, z, time-of-day)
     REAL, allocatable, DIMENSION(:,:,:,:) :: counter !< coordinates of the buffer and counter are (x,y,z,time-of-day)
     ! the following two counters are used in time-averaging for some
     ! combination of the field options. Their size is the length of the
     ! diurnal axis; the counters must be tracked separately for each of
     ! the diurnal interval, because the number of time slices accumulated
     ! in each can be different, depending on time step and the number of
     ! diurnal samples.
     REAL, allocatable, DIMENSION(:)  :: count_0d !< the following two counters are used in time-averaging for some
     !! combination of the field options. Their size is the length of the
     !! diurnal axis; the counters must be tracked separately for each of
     !! the diurnal interval, because the number of time slices accumulated
     !! in each can be different, depending on time step and the number of
     !! diurnal samples.
     INTEGER, allocatable, dimension(:) :: num_elements !< the following two counters are used in time-averaging
     !! for some combination of the field options. Their size is the length of the
     !! diurnal axis; the counters must be tracked separately for each of
     !! the diurnal interval, because the number of time slices accumulated
     !! in each can be different, depending on time step and the number of
     !! diurnal samples.

     TYPE(time_type) :: last_output, next_output, next_next_output
     TYPE(diag_fieldtype) :: f_type
     INTEGER, DIMENSION(4) :: axes
     INTEGER :: num_axes, total_elements, region_elements
     INTEGER :: n_diurnal_samples !< number of diurnal sample intervals, 1 or more
     TYPE(diag_grid) :: output_grid
     LOGICAL :: local_output, need_compute, phys_window, written_once
     LOGICAL :: reduced_k_range
     TYPE(fmsDiagIbounds_type) :: buff_bounds
     TYPE(time_type) :: Time_of_prev_field_data
     TYPE(diag_atttype), allocatable, dimension(:) :: attributes
     INTEGER :: num_attributes
!----------
!ug support
     logical :: reduced_k_unstruct = .false.
!----------
  END TYPE output_field_type

  !> @brief Type to hold the diagnostic axis description.
  !> @ingroup diag_data_mod
  TYPE diag_axis_type
     CHARACTER(len=128) :: name
     CHARACTER(len=256) :: units, long_name
     CHARACTER(len=1) :: cart_name
     REAL, DIMENSION(:), POINTER :: diag_type_data
     INTEGER, DIMENSION(MAX_SUBAXES) :: start
     INTEGER, DIMENSION(MAX_SUBAXES) :: end
     CHARACTER(len=128), DIMENSION(MAX_SUBAXES) :: subaxis_name
     INTEGER :: length, direction, edges, set, shift
     TYPE(domain1d) :: Domain
     TYPE(domain2d) :: Domain2
     TYPE(domain2d), dimension(MAX_SUBAXES) :: subaxis_domain2
     type(domainUG) :: DomainUG
     CHARACTER(len=128) :: aux, req
     INTEGER :: tile_count
     TYPE(diag_atttype), allocatable, dimension(:) :: attributes !< Array to hold user definable attributes
     INTEGER :: num_attributes !< Number of defined attibutes
     INTEGER :: domain_position !< The position in the doman (NORTH or EAST or CENTER)
  END TYPE diag_axis_type

  !> @ingroup diag_data_mod
  TYPE diag_global_att_type
     CHARACTER(len=128)   :: grid_type='regular'
     CHARACTER(len=128)   :: tile_name='N/A'
  END TYPE diag_global_att_type

  !> @brief Type to hold the attributes of the field/axis/file
  !> @ingroup diag_data_mod
  type fmsDiagAttribute_type
    class(*), allocatable         :: att_value(:) !< Value of the attribute
    character(len=:), allocatable :: att_name     !< Name of the attribute
    contains
      procedure :: add => fms_add_attribute
      procedure :: write_metadata
  end type fmsDiagAttribute_type
! Include variable "version" to be written to log file.
#include<file_version.h>

  !> @addtogroup diag_data_mod
  !> @{

  ! <!-- Other public variables -->
  INTEGER :: num_files = 0 !< Number of output files currenly in use by the diag_manager.
  INTEGER :: num_input_fields = 0 !< Number of input fields in use.
  INTEGER :: num_output_fields = 0 !< Number of output fields in use.
  INTEGER :: null_axis_id

  ! <!-- Namelist variables -->
  LOGICAL :: append_pelist_name = .FALSE.
  LOGICAL :: mix_snapshot_average_fields =.FALSE.
  INTEGER :: max_files = 31 !< Maximum number of output files allowed.  Increase via diag_manager_nml.
  INTEGER :: max_output_fields = 300 !< Maximum number of output fields.  Increase via diag_manager_nml.
  INTEGER :: max_input_fields = 600 !< Maximum number of input fields.  Increase via diag_manager_nml.
  INTEGER :: max_out_per_in_field = 150 !< Maximum number of output_fields per input_field.  Increase
                                        !! via diag_manager_nml.
  INTEGER :: max_axes = 60 !< Maximum number of independent axes.
  LOGICAL :: do_diag_field_log = .FALSE.
  LOGICAL :: write_bytes_in_file = .FALSE.
  LOGICAL :: debug_diag_manager = .FALSE.
  LOGICAL :: flush_nc_files = .FALSE. !< Control if diag_manager will force a
                                      !! flush of the netCDF file on each write.
                                      !! Note: changing this to .TRUE. can greatly
                                      !! reduce the performance of the model, as the
                                      !! model must wait until the flush to disk has
                                      !! completed.
  INTEGER :: max_num_axis_sets = 25
  LOGICAL :: use_cmor = .FALSE. !< Indicates if we should overwrite the MISSING_VALUE to use the CMOR missing value.
  LOGICAL :: issue_oor_warnings = .TRUE. !< Issue warnings if the output field has values outside the given
                                         !! range for a variable.
  LOGICAL :: oor_warnings_fatal = .FALSE. !< Cause a fatal error if the output field has a value outside the
                                          !! given range for a variable.
  LOGICAL :: region_out_use_alt_value = .TRUE. !< Will determine which value to use when checking a regional
                                               !! output if the region is the full axis or a sub-axis.
                                               !! The values are defined as <TT>GLO_REG_VAL</TT>
                                               !! (-999) and <TT>GLO_REG_VAL_ALT</TT> (-1) in <TT>diag_data_mod</TT>.

  INTEGER :: max_field_attributes = 4 !< Maximum number of user definable attributes per field. Liptak:
                                      !! Changed from 2 to 4 20170718
  INTEGER :: max_file_attributes = 2 !< Maximum number of user definable global attributes per file.
  INTEGER :: max_axis_attributes = 4 !< Maximum number of user definable attributes per axis.
  LOGICAL :: prepend_date = .TRUE. !< Should the history file have the start date prepended to the file name.
                                   !! <TT>.TRUE.</TT> is only supported if the diag_manager_init
                                   !! routine is called with the optional time_init parameter.
  LOGICAL :: use_mpp_io = .false. !< false is fms2_io (default); true is mpp_io
  LOGICAL :: use_refactored_send = .false. !< Namelist flag to use refactored send_data math funcitons.
  LOGICAL :: use_modern_diag = .false. !< Namelist flag to use the modernized diag_manager code
  LOGICAL :: use_clock_average = .false. !< .TRUE. if the averaging of variable is done based on the clock
                                         !! For example, if doing daily averages and your start the simulation in
                                         !! day1_hour3, it will do the average between day1_hour3 to day2_hour 0
                                         !! the default behavior will do the average between day1 hour3 to day2 hour3
  ! <!-- netCDF variable -->

  REAL :: FILL_VALUE = NF_FILL_REAL !< Fill value used.  Value will be <TT>NF90_FILL_REAL</TT> if using the
                                    !! netCDF module, otherwise will be 9.9692099683868690e+36.
                                    ! from file /usr/local/include/netcdf.inc

  !! @note `pack_size` and `pack_size_str` are set in diag_manager_init depending on how FMS was compiled
  !! if FMS was compiled with default reals as 64bit, it will be set to 1 and "double",
  !! if FMS was compiled with default reals as 32bit, it will set to 2 and "float"
  !! The time variables will written in the precision defined by `pack_size_str`
  !! This is to reproduce previous diag manager behavior.
  !TODO This may not be mixed precision friendly
  INTEGER :: pack_size = 1 !< 1 for double and 2 for float
  CHARACTER(len=6) :: pack_size_str="double" !< Pack size as a string to be used in fms2_io register call
                                             !! set to "double" or "float"

  ! <!-- REAL public variables -->
  REAL(r8_kind) :: EMPTY = 0.0
  REAL(r8_kind) :: MAX_VALUE, MIN_VALUE

  ! <!-- Global data for all files -->
  TYPE(time_type) :: diag_init_time !< Time diag_manager_init called.  If init_time not included in
                                    !! diag_manager_init call, then same as base_time
  TYPE(time_type), private :: base_time !< The base_time read from diag_table
  logical, private :: base_time_set !< Flag indicating that the base_time is set
                                    !! This is to prevent users from calling set_base_time multiple times
  INTEGER, private :: base_year, base_month, base_day, base_hour, base_minute, base_second
  CHARACTER(len = 256):: global_descriptor

  ! <!-- ALLOCATABLE variables -->
  TYPE(file_type), SAVE, ALLOCATABLE :: files(:)
  TYPE(input_field_type), ALLOCATABLE :: input_fields(:)
  TYPE(output_field_type), ALLOCATABLE :: output_fields(:)
  ! used if use_mpp_io = .false.
  type(FmsNetcdfUnstructuredDomainFile_t),allocatable, target :: fileobjU(:)
  type(FmsNetcdfDomainFile_t),allocatable, target :: fileobj(:)
  type(FmsNetcdfFile_t),allocatable, target :: fileobjND(:)
  character(len=2),allocatable :: fnum_for_domain(:) !< If this file number in the array is for the "unstructured"
                                                     !! or "2d" domain

  ! <!-- Even More Variables -->
  TYPE(time_type) :: time_zero
  LOGICAL :: first_send_data_call = .TRUE.
  LOGICAL :: module_is_initialized = .FALSE. !< Indicate if diag_manager has been initialized
  INTEGER :: diag_log_unit
  CHARACTER(len=10), DIMENSION(6) :: time_unit_list = (/'seconds   ', 'minutes   ',&
       & 'hours     ', 'days      ', 'months    ', 'years     '/)
  CHARACTER(len=32) :: pelist_name
  INTEGER :: oor_warning = WARNING

CONTAINS

  !> @brief Initialize and write the version number of this file to the log file.
  SUBROUTINE diag_data_init()
    IF (module_is_initialized) THEN
       RETURN
    END IF

    ! Write version number out to log file
    call write_version_number("DIAG_DATA_MOD", version)
    module_is_initialized = .true.
    base_time_set = .false.

  END SUBROUTINE diag_data_init

  !> @brief Set the module variable base_time
  subroutine set_base_time(base_time_int)
    integer :: base_time_int(6) !< base_time as an array [year month day hour min sec]

    CHARACTER(len=9) :: amonth !< Month name
    INTEGER :: stdlog_unit !< Fortran file unit number for the stdlog file.

    if (.not. module_is_initialized) call mpp_error(FATAL, "set_base_time: diag_data is not initialized")
    if (base_time_set) call mpp_error(FATAL, "set_base_time: the base_time is already set!")

    base_year = base_time_int(1)
    base_month = base_time_int(2)
    base_day = base_time_int(3)
    base_hour = base_time_int(4)
    base_minute = base_time_int(5)
    base_second = base_time_int(6)

    ! Set up the time type for base time
    IF ( get_calendar_type() /= NO_CALENDAR ) THEN
      IF ( base_year==0 .OR. base_month==0 .OR. base_day==0 ) THEN
         call mpp_error(FATAL, 'diag_data_mod::set_base_time'//&
            &  'The base_year/month/day can not equal zero')
      END IF
      base_time = set_date(base_year, base_month, base_day, base_hour, base_minute, base_second)
      amonth = month_name(base_month)
    ELSE
      ! No calendar - ignore year and month
      base_time = set_time(NINT(base_hour*SECONDS_PER_HOUR)+NINT(base_minute*SECONDS_PER_MINUTE)+base_second, &
                          &  base_day)
      base_year = 0
      base_month = 0
      amonth = 'day'
    END IF

    ! get the stdlog unit number
    stdlog_unit = stdlog()

    IF ( mpp_pe() == mpp_root_pe() ) THEN
      WRITE (stdlog_unit,'("base date used = ",I4,1X,A,2I3,2(":",I2.2)," gmt")') base_year, TRIM(amonth), base_day, &
           & base_hour, base_minute, base_second
    END IF

    base_time_set = .true.

  end subroutine set_base_time

  !> @brief gets the module variable base_time
  !> @return the base_time
  function get_base_time() &
  result(res)
     TYPE(time_type) :: res
     res = base_time
  end function get_base_time

  !> @brief gets the module variable base_year
  !> @return the base_year
  function get_base_year() &
    result(res)
    integer :: res
    res = base_year
  end function get_base_year

  !> @brief gets the module variable base_month
  !> @return the base_month
  function get_base_month() &
    result(res)
    integer :: res
    res = base_month
  end function get_base_month

  !> @brief gets the module variable base_day
  !> @return the base_day
  function get_base_day() &
    result(res)
    integer :: res
    res = base_day
  end function get_base_day

  !> @brief gets the module variable base_hour
  !> @return the base_hour
  function get_base_hour() &
    result(res)
    integer :: res
    res = base_hour
  end function get_base_hour

  !> @brief gets the module variable base_minute
  !> @return the base_minute
  function get_base_minute() &
    result(res)
    integer :: res
    res = base_minute
  end function get_base_minute

  !> @brief gets the module variable base_second
  !> @return the base_second
  function get_base_second() &
    result(res)
    integer :: res
    res = base_second
  end function get_base_second

  !> @brief Adds an attribute to the attribute type
  subroutine fms_add_attribute(this, att_name, att_value)
    class(fmsDiagAttribute_type), intent(inout) :: this         !< Diag attribute type
    character(len=*),             intent(in)    :: att_name     !< Name of the attribute
    class(*),                     intent(in)    :: att_value(:) !< The attribute value to add

    integer :: natt !< the size of att_value

    natt = size(att_value)
    this%att_name = att_name
    select type (att_value)
    type is (integer(kind=i4_kind))
      allocate(integer(kind=i4_kind) :: this%att_value(natt))
      this%att_value = att_value
    type is (integer(kind=i8_kind))
      allocate(integer(kind=i8_kind) :: this%att_value(natt))
      this%att_value = att_value
    type is (real(kind=r4_kind))
      allocate(real(kind=r4_kind) :: this%att_value(natt))
      this%att_value = att_value
    type is (real(kind=r8_kind))
      allocate(real(kind=r8_kind) :: this%att_value(natt))
      this%att_value = att_value
    type is (character(len=*))
      allocate(character(len=len(att_value)) :: this%att_value(natt))
      select type(aval => this%att_value)
        type is (character(len=*))
          aval = att_value
      end select
    end select
  end subroutine fms_add_attribute

  !> @brief gets the type of a variable
  !> @return the type of the variable (r4,r8,i4,i8,string)
  function get_var_type(var) &
  result(var_type)
    class(*), intent(in) :: var      !< Variable to get the type for
    integer              :: var_type !< The variable's type

    select type(var)
    type is (real(r4_kind))
      var_type = r4
    type is (real(r8_kind))
      var_type = r8
    type is (integer(i4_kind))
      var_type = i4
    type is (integer(i8_kind))
      var_type = i8
    type is (character(len=*))
      var_type = string
    class default
      call mpp_error(FATAL, "get_var_type:: The variable does not have a supported type. &
                            &The supported types are r4, r8, i4, i8 and string.")
    end select
  end function get_var_type

  !> @brief Writes out the attributes from an fmsDiagAttribute_type
  subroutine write_metadata(this, fileobj, var_name, cell_methods)
    class(fmsDiagAttribute_type),      intent(inout) :: this          !< Diag attribute type
    class(FmsNetcdfFile_t),            INTENT(INOUT) :: fileobj       !< Fms2_io fileobj to write to
    character(len=*),                  intent(in)    :: var_name      !< The name of the variable to write to
    character(len=*),        optional, intent(inout) :: cell_methods  !< The cell methods attribute

    select type (att_value =>this%att_value)
    type is (character(len=*))
      !< If the attribute is cell methods append to the current cell_methods attribute value
      !! This will be writen once all of the cell_methods attributes are gathered ...
      if (present(cell_methods)) then
        if (trim(this%att_name) .eq. "cell_methods") then
          cell_methods = trim(cell_methods)//" "//trim(att_value(1))
          return
        endif
      endif

      call register_variable_attribute(fileobj, var_name, this%att_name, trim(att_value(1)), &
                                       str_len=len_trim(att_value(1)))
    type is (real(kind=r8_kind))
      call register_variable_attribute(fileobj, var_name, this%att_name, real(att_value, kind=r8_kind))
    type is (real(kind=r4_kind))
      call register_variable_attribute(fileobj, var_name, this%att_name, real(att_value, kind=r4_kind))
    type is (integer(kind=i4_kind))
      call register_variable_attribute(fileobj, var_name, this%att_name, int(att_value, kind=i4_kind))
    type is (integer(kind=i8_kind))
      call register_variable_attribute(fileobj, var_name, this%att_name, int(att_value, kind=i8_kind))
    end select

  end subroutine write_metadata
END MODULE diag_data_mod
!> @}
! close documentation grouping
