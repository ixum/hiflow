  if (WITH_METIS)
set(mesh_SOURCES
  advanced_iteration_demo
  cell_type_demo
  custom_refine_demo
  ghost_cells_demo
  global_partitioning_demo
  iteration_demo
  local_refinements_by_attribute
  mesh_tools_partitioning_demo
  mesh_tools_reader_demo
  moving_mesh_demo
  refine_by_attribute
  shared_vertex_table_demo
  adapt_mesh_to_function_demo
  scaling_test
  )
else()
set(mesh_SOURCES
  advanced_iteration_demo
  cell_type_demo
  custom_refine_demo
  ghost_cells_demo
  iteration_demo
  local_refinements_by_attribute
  mesh_tools_partitioning_demo
  mesh_tools_reader_demo
  moving_mesh_demo
  refine_by_attribute
  shared_vertex_table_demo
  adapt_mesh_to_function_demo
  scaling_test
  )
 endif()

set(mesh_ADDITIONAL_FILES
  scaling_test.xml
  )