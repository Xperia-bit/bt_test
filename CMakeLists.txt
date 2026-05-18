cmake_minimum_required(VERSION 3.20.0)
set(DTC_OVERLAY_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/boards/xiao_nrf54l15_nrf54l15_cpuapp.overlay
  CACHE STRING "Devicetree overlay file"
  FORCE
)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt_test)

target_sources(app PRIVATE
  src/main.c
  src/imu.c
  src/sensor.c
)
