// Copyright (C) 2011-2017 Vincent Heuveline
//
// HiFlow3 is free software: you can redistribute it and/or modify it under the
// terms of the European Union Public Licence (EUPL) v1.2 as published by the
// European Union or (at your option) any later version.
//
// HiFlow3 is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the European Union Public Licence (EUPL) v1.2 for more
// details.
//
// You should have received a copy of the European Union Public Licence (EUPL) v1.2
// along with HiFlow3.  If not, see <https://joinup.ec.europa.eu/page/eupl-text-11-12>.

#include "facet_visualization.h"

#include "common/log.h"
#include "fem/cell_transformation.h"
#include "mesh/entity.h"
#include "mesh/iterator.h"
#include "space/vector_space.h"
#include "mesh/attributes.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <tinyxml.h>

const int DEBUG_LEVEL = 1;

namespace hiflow
{

    //////////////// FacetVisualization ////////////////////////////////////////////

    template<class DataType>
    FacetVisualization<DataType>::FacetVisualization ( const VectorSpace<DataType>& space, std::vector<int>& material_numbers )
    : space_ ( space ), material_numbers_ ( material_numbers )
    {
        parallel_visualization_ = space_.mesh ( ).has_attribute ( "_remote_index_", space_.mesh ( ).tdim ( ) );

        const mesh::Mesh& mesh = space_.mesh ( );
        const mesh::TDim tdim = mesh.tdim ( );

        this->num_visu_points_ = 0;
        this->num_visu_cells_ = 0;

        for ( EntityIterator it = mesh.begin ( tdim ), end_it = mesh.end ( tdim );
              it != end_it; ++it )
        {

            if ( parallel_visualization_ )
            {
                int rem_ind = -100;
                it->get<int>( "_remote_index_", &rem_ind );
                if ( rem_ind != -1 ) continue;
            }

            for ( IncidentEntityIterator f_it = it->begin_incident ( tdim - 1 ), f_it_end = it->end_incident ( tdim - 1 );
                  f_it != f_it_end;
                  ++f_it )
            {

                if ( std::find ( this->material_numbers_.begin ( ), this->material_numbers_.end ( ), f_it->get_material_number ( ) ) == this->material_numbers_.end ( ) )
                {
                    continue;
                }

                this->num_visu_points_ += f_it->num_vertices ( );
                this->num_visu_cells_ += 1;

            }

        }

    }

    template<class DataType>
    void FacetVisualization<DataType>::visualize ( const EvalFunction& fun, const std::string& name )
    {
        const mesh::Mesh& mesh = space_.mesh ( );
        const mesh::TDim tdim = mesh.tdim ( );

        std::vector<DataType> values, cell_values;
        values.reserve ( this->num_visu_points_ );

        for ( EntityIterator it = mesh.begin ( tdim ), end_it = mesh.end ( tdim );
              it != end_it; ++it )
        {

            if ( parallel_visualization_ )
            {
                int rem_ind = -100;
                it->get<int>( "_remote_index_", &rem_ind );
                if ( rem_ind != -1 ) continue;
            }

            const doffem::CellTransformation<DataType>& cell_trans = space_.GetCellTransformation ( *it );

            for ( IncidentEntityIterator f_it = it->begin_incident ( tdim - 1 ), f_it_end = it->end_incident ( tdim - 1 );
                  f_it != f_it_end;
                  ++f_it )
            {

                if ( std::find ( this->material_numbers_.begin ( ), this->material_numbers_.end ( ), f_it->get_material_number ( ) ) == this->material_numbers_.end ( ) )
                {
                    continue;
                }

                cell_values.clear ( );
                cell_values.resize ( f_it->num_vertices ( ), 1.e32 );

                std::vector<DataType> coords;
                f_it->get_coordinates ( coords );

                if ( tdim == 3 )
                {
                    for ( int i = 0, i_end = coords.size ( ) / tdim; i != i_end; ++i )
                    {
                        std::vector<DataType> ref_coords ( tdim );
                        cell_trans.inverse ( coords[i * tdim + 0], coords[i * tdim + 1], coords[i * tdim + 2],
                                             ref_coords[0], ref_coords[1], ref_coords[2] );

                        fun ( *it, ref_coords, cell_values );
                        values.insert ( values.end ( ), cell_values.begin ( ), cell_values.end ( ) );
                    }
                }
                else if ( tdim == 2 )
                {
                    for ( int i = 0, i_end = coords.size ( ) / tdim; i != i_end; ++i )
                    {
                        std::vector<DataType> ref_coords ( tdim );
                        cell_trans.inverse ( coords[i * tdim + 0], coords[i * tdim + 1],
                                             ref_coords[0], ref_coords[1] );

                        fun ( *it, ref_coords, cell_values );
                        values.insert ( values.end ( ), cell_values.begin ( ), cell_values.end ( ) );
                    }
                }
                else if ( tdim == 1 )
                {
                    for ( int i = 0, i_end = coords.size ( ) / tdim; i != i_end; ++i )
                    {
                        std::vector<DataType> ref_coords ( tdim );
                        cell_trans.inverse ( coords[i * tdim + 0], ref_coords[0] );

                        fun ( *it, ref_coords, cell_values );
                        values.insert ( values.end ( ), cell_values.begin ( ), cell_values.end ( ) );
                    }
                }

            }

        }

        functions_.insert ( std::make_pair ( name, values ) );
    }

    template<class DataType>
    void FacetVisualization<DataType>::visualize_cell_data ( const std::vector<DataType>& cell_data, const std::string& name )
    {
        const mesh::Mesh& mesh = space_.mesh ( );
        const mesh::TDim tdim = mesh.tdim ( );

        std::vector<DataType> values;

        for ( EntityIterator it = mesh.begin ( tdim ), end_it = mesh.end ( tdim );
              it != end_it; ++it )
        {

            if ( parallel_visualization_ )
            {
                int rem_ind = -100;
                it->get<int>( "_remote_index_", &rem_ind );
                if ( rem_ind != -1 ) continue;
            }

            for ( IncidentEntityIterator f_it = it->begin_incident ( tdim - 1 ), f_it_end = it->end_incident ( tdim - 1 );
                  f_it != f_it_end;
                  ++f_it )
            {

                if ( std::find ( this->material_numbers_.begin ( ), this->material_numbers_.end ( ), f_it->get_material_number ( ) ) == this->material_numbers_.end ( ) )
                {
                    continue;
                }

                values.insert ( values.end ( ), 1, cell_data.at ( f_it->index ( ) ) );
            }

        }

        functions_cell_.insert ( std::make_pair ( name, values ) );
    }

    template<class DataType>
    void FacetVisualization<DataType>::write ( const std::string& filename ) const
    {
        const mesh::Mesh& mesh = space_.mesh ( );
        const mesh::TDim tdim = mesh.tdim ( );
        const mesh::GDim gdim = mesh.gdim ( );

        TiXmlDocument doc;
        TiXmlDeclaration* decl = new TiXmlDeclaration ( "1.0", "", "" );
        doc.LinkEndChild ( decl );

        TiXmlElement* vtkFile = new TiXmlElement ( "VTKFile" );
        vtkFile->SetAttribute ( "type", "UnstructuredGrid" );
        vtkFile->SetAttribute ( "version", "0.1" );
        vtkFile->SetAttribute ( "byte_order", "LittleEndian" );
        vtkFile->SetAttribute ( "compressor", "vtkZLibDataCompressor" );
        doc.LinkEndChild ( vtkFile );

        TiXmlElement* umesh = new TiXmlElement ( "UnstructuredGrid" );
        vtkFile->LinkEndChild ( umesh );

        TiXmlElement* piece = new TiXmlElement ( "Piece" );
        piece->SetAttribute ( "NumberOfPoints", this->num_visu_points_ );
        piece->SetAttribute ( "NumberOfCells", this->num_visu_cells_ );
        umesh->LinkEndChild ( piece );

        // Scratch variables for data.
        TiXmlElement* data_array;
        std::stringstream os;

        //// Write points ////////////////////////////////////////////////////////////
        TiXmlElement* points = new TiXmlElement ( "Points" );
        piece->LinkEndChild ( points );
        data_array = new TiXmlElement ( "DataArray" );

        // Set correct length of float in dependence of DataType
        std::ostringstream type_float;
        type_float << "Float" << sizeof (DataType ) * 8;
        data_array->SetAttribute ( "type", type_float.str ( ) );

        data_array->SetAttribute ( "Name", "Array" );
        // always 3 comps, since vtk doesn:t handle 2D.
        data_array->SetAttribute ( "NumberOfComponents", "3" );
        data_array->SetAttribute ( "format", "ascii" );

        std::vector<DataType> ref_pt ( gdim, 0. ), mapped_pt ( 3, 0. );
        DataType range_min = std::numeric_limits<DataType>::max ( );
        DataType range_max = std::numeric_limits<DataType>::min ( );
        int cell_type_min = std::numeric_limits<int>::max ( );
        int cell_type_max = std::numeric_limits<int>::min ( );

        for ( EntityIterator it = mesh.begin ( tdim ), end_it = mesh.end ( tdim ); it != end_it; ++it )
        {

            if ( parallel_visualization_ )
            {
                int rem_ind = -100;
                it->get<int>( "_remote_index_", &rem_ind );
                if ( rem_ind != -1 ) continue;
            }
            const doffem::CellTransformation<DataType>& cell_trans = space_.GetCellTransformation ( *it );

            for ( IncidentEntityIterator f_it = it->begin_incident ( tdim - 1 ), f_it_end = it->end_incident ( tdim - 1 );
                  f_it != f_it_end;
                  ++f_it )
            {

                if ( std::find ( this->material_numbers_.begin ( ), this->material_numbers_.end ( ), f_it->get_material_number ( ) ) == this->material_numbers_.end ( ) )
                {
                    continue;
                }

                for ( int p = 0, p_end = f_it->num_vertices ( ); p != p_end; ++p )
                {
                    const int offset = gdim * p;

                    std::vector<DataType> coords;
                    f_it->get_coordinates ( coords );

                    for ( int c = 0; c != gdim; ++c )
                    {
                        mapped_pt[c] = coords[p * tdim + c];
                    }

                    for ( int c = 0; c < 3; ++c )
                    {
                        range_min = std::min ( range_min, mapped_pt[c] );
                        range_max = std::max ( range_max, mapped_pt[c] );
                        os << mapped_pt[c] << " ";
                    }

                }

            }

        }

        TiXmlText* coords = new TiXmlText ( os.str ( ) );
        data_array->SetAttribute ( "RangeMin", range_min );
        data_array->SetAttribute ( "RangeMax", range_max );

        points->LinkEndChild ( data_array );
        data_array->LinkEndChild ( coords );
        os.str ( "" );
        os.clear ( );
        //// End write points /////////////////////////////////////////////////////////

        //// Write cells //////////////////////////////////////////////////////////////
        TiXmlElement* cells = new TiXmlElement ( "Cells" );
        piece->LinkEndChild ( cells );

        int p_offset = 0, cell_offset = 0;

        // Connectivity, Offsets, and Types arrays
        std::ostringstream off_os, type_os;
        static const int vtk_cell_types[] = { 1, 3, 5, 9, 10, 12, 14 };

        for ( EntityIterator it = mesh.begin ( tdim ); it != mesh.end ( tdim ); ++it )
        {

            if ( parallel_visualization_ )
            {
                int rem_ind = -100;
                it->get<int>( "_remote_index_", &rem_ind );
                if ( rem_ind != -1 ) continue;
            }

            for ( IncidentEntityIterator f_it = it->begin_incident ( tdim - 1 ), f_it_end = it->end_incident ( tdim - 1 );
                  f_it != f_it_end;
                  ++f_it )
            {

                if ( std::find ( this->material_numbers_.begin ( ), this->material_numbers_.end ( ), f_it->get_material_number ( ) ) == this->material_numbers_.end ( ) )
                {
                    continue;
                }

                for ( int v = 0, end_v = f_it->num_vertices ( ); v != end_v; ++v )
                {
                    os << v + p_offset << " ";
                }

                cell_offset += f_it->num_vertices ( );
                off_os << cell_offset << " ";
                type_os << vtk_cell_types[static_cast < int > ( f_it->cell_type ( ).tag ( ) )] << " ";
                cell_type_min = std::min ( cell_type_min, vtk_cell_types[static_cast < int > ( f_it->cell_type ( ).tag ( ) )] );
                cell_type_max = std::max ( cell_type_max, vtk_cell_types[static_cast < int > ( f_it->cell_type ( ).tag ( ) )] );

                p_offset += f_it->num_vertices ( );
            }

        }

        data_array = new TiXmlElement ( "DataArray" );
        data_array->SetAttribute ( "type", "Int64" );
        data_array->SetAttribute ( "Name", "connectivity" );
        data_array->SetAttribute ( "format", "ascii" );
        data_array->SetAttribute ( "RangeMin", 0 );
        data_array->SetAttribute ( "RangeMax", this->num_visu_points_ );

        TiXmlText* conns = new TiXmlText ( os.str ( ) );
        data_array->LinkEndChild ( conns );
        cells->LinkEndChild ( data_array );
        os.str ( "" );
        os.clear ( );

        data_array = new TiXmlElement ( "DataArray" );
        data_array->SetAttribute ( "type", "Int64" );
        data_array->SetAttribute ( "Name", "offsets" );
        data_array->SetAttribute ( "format", "ascii" );
        data_array->SetAttribute ( "RangeMin", 0 );
        data_array->SetAttribute ( "RangeMax", cell_offset );

        TiXmlText* offs = new TiXmlText ( off_os.str ( ) );
        data_array->LinkEndChild ( offs );
        cells->LinkEndChild ( data_array );
        off_os.str ( "" );
        off_os.clear ( );

        data_array = new TiXmlElement ( "DataArray" );
        data_array->SetAttribute ( "type", "UInt8" );
        data_array->SetAttribute ( "Name", "types" );
        data_array->SetAttribute ( "format", "ascii" );
        data_array->SetAttribute ( "RangeMin", cell_type_min );
        data_array->SetAttribute ( "RangeMax", cell_type_max );

        TiXmlText* types = new TiXmlText ( type_os.str ( ) );
        data_array->LinkEndChild ( types );
        cells->LinkEndChild ( data_array );
        type_os.str ( "" );
        type_os.clear ( );

        //// End Write cells //////////////////////////////////////////////////////////

        // Write point data /////////////////////////////////////////////////////////
        TiXmlElement* point_data = new TiXmlElement ( "PointData" );
        piece->LinkEndChild ( point_data );

        for ( typename std::map<std::string, std::vector<DataType> >::const_iterator it = functions_.begin ( ),
              end_it = functions_.end ( ); it != end_it; ++it )
        {
            data_array = new TiXmlElement ( "DataArray" );
            data_array->SetAttribute ( "Name", it->first );
            data_array->SetAttribute ( "type", type_float.str ( ) );
            data_array->SetAttribute ( "format", "ascii" );

            for ( int i = 0, end_i = it->second.size ( ); i != end_i; ++i )
            {
                os << ( it->second )[i] << " ";
            }

            TiXmlText* data = new TiXmlText ( os.str ( ) );
            data_array->LinkEndChild ( data );
            point_data->LinkEndChild ( data_array );
            os.str ( "" );
            os.clear ( );
        }

        TiXmlElement* cell_data = new TiXmlElement ( "CellData" );
        piece->LinkEndChild ( cell_data );

        for ( typename std::map<std::string, std::vector<DataType> >::const_iterator it = functions_cell_.begin ( ),
              end_it = functions_cell_.end ( ); it != end_it; ++it )
        {
            data_array = new TiXmlElement ( "DataArray" );
            data_array->SetAttribute ( "Name", it->first );
            data_array->SetAttribute ( "type", type_float.str ( ) );
            data_array->SetAttribute ( "format", "ascii" );

            for ( int i = 0, end_i = it->second.size ( ); i != end_i; ++i )
            {
                os << ( it->second )[i] << " ";
            }

            TiXmlText* data = new TiXmlText ( os.str ( ) );
            data_array->LinkEndChild ( data );
            cell_data->LinkEndChild ( data_array );
            os.str ( "" );
            os.clear ( );
        }

        doc.SaveFile ( filename );
    }

    template class FacetVisualization<double>;
    template class FacetVisualization<float>;

    //////// ParallelFacetVisualization ////////////////////////

    template<class DataType>
    void ParallelFacetVisualization<DataType>::write ( const std::string& filename, const std::string& path ) const
    {
        //const mesh::Mesh& mesh = this->space_.mesh();
        //const mesh::TDim tdim = mesh.tdim();
        //const mesh::GDim gdim = mesh.gdim();
        // get MPI rank
        int rank = -1, num_procs = -1;
        MPI_Comm_rank ( mpi_comm_, &rank );
        MPI_Comm_size ( mpi_comm_, &num_procs );

        std::stringstream s;
        s << rank;

        // get the correct filename including the path
        std::istringstream filename_root_dir ( filename );

        std::size_t dir = filename_root_dir.str ( ).find_last_of ( "." );
        LOG_DEBUG ( 3, "Filename: " << filename );

        std::string filename_without_suffix = filename_root_dir.str ( ).substr ( 0, dir );

        assert ( !filename_without_suffix.empty ( ) );

        std::string str_src_filename = ( path + filename_without_suffix + "_" + s.str ( ) + ".vtu" );
        LOG_DEBUG ( 3, "Filename without suffix: " << filename_without_suffix );
        assert ( !str_src_filename.empty ( ) );

        // Each process writes its vtu file
        FacetVisualization<DataType>::write ( str_src_filename );

        // Master writes pvtu file
        if ( rank == master_rank_ )
        {
            TiXmlDocument doc;
            TiXmlDeclaration* decl = new TiXmlDeclaration ( "1.0", "", "" );
            doc.LinkEndChild ( decl );

            TiXmlElement* vtkFile = new TiXmlElement ( "VTKFile" );
            vtkFile->SetAttribute ( "type", "PUnstructuredGrid" );
            vtkFile->SetAttribute ( "version", "0.1" );
            vtkFile->SetAttribute ( "byte_order", "LittleEndian" );
            vtkFile->SetAttribute ( "compressor", "vtkZLibDataCompressor" );
            doc.LinkEndChild ( vtkFile );

            TiXmlElement* pumesh = new TiXmlElement ( "PUnstructuredGrid" );
            // GhostLevel in PUnstructuredGrid is always 0
            pumesh->SetAttribute ( "GhostLevel", 0 );
            vtkFile->LinkEndChild ( pumesh );

            TiXmlElement* p_point_data = new TiXmlElement ( "PPointData" );
            pumesh->LinkEndChild ( p_point_data );

            // Set correct length of float in dependence of DataType
            std::ostringstream type_float;
            type_float << "Float" << sizeof (DataType ) * 8;

            TiXmlElement* p_data_array;
            for ( typename std::map<std::string, std::vector<DataType> >::const_iterator it = this->functions_.begin ( ),
                  end_it = this->functions_.end ( ); it != end_it; ++it )
            {

                p_data_array = new TiXmlElement ( "PDataArray" );
                p_data_array->SetAttribute ( "Name", it->first );
                p_data_array->SetAttribute ( "type", type_float.str ( ) );
                p_data_array->SetAttribute ( "format", "ascii" );
                p_point_data->LinkEndChild ( p_data_array );
            }

            TiXmlElement* p_cell_data = new TiXmlElement ( "PCellData" );
            pumesh->LinkEndChild ( p_cell_data );
            //int tdim = mesh.tdim();

            // write cell data
            // TODO currently only Float64 is supported
            TiXmlElement* data_array;
            for ( typename std::map< std::string, std::vector<DataType> >::const_iterator it = this->functions_cell_.begin ( );
                  it != this->functions_cell_.end ( ); ++it )
            {
                data_array = new TiXmlElement ( "PDataArray" );
                data_array->SetAttribute ( "Name", it->first );
                data_array->SetAttribute ( "type", type_float.str ( ) );
                data_array->SetAttribute ( "format", "ascii" );
                p_cell_data->LinkEndChild ( data_array );
                p_cell_data->SetAttribute ( "Scalars", it->first );
            }

            // NB: This has to be AFTER the the other elements, since
            // the same order in the vtu and pvtu file is needed!

            TiXmlElement* p_points = new TiXmlElement ( "PPoints" );
            pumesh->LinkEndChild ( p_points );

            TiXmlElement* p_points_data_array = new TiXmlElement ( "PDataArray" );
            p_points_data_array->SetAttribute ( "type", type_float.str ( ) );
            p_points_data_array->SetAttribute ( "NumberOfComponents", "3" );
            p_points->LinkEndChild ( p_points_data_array );

            // get the correct filename without the path
            std::size_t pos = filename_root_dir.str ( ).find_last_of ( "/\\" );
            assert ( !filename_root_dir.str ( ).substr ( pos + 1, filename_root_dir.str ( ).length ( ) ).empty ( ) );

            std::stringstream str_proc_id;

            for ( int proc_id = 0; proc_id < num_procs; ++proc_id )
            {
                TiXmlElement* piece = new TiXmlElement ( "Piece" ); // needs to be inside the loop!
                str_proc_id << proc_id;
                piece->SetAttribute ( "Source", filename_root_dir.str ( ).substr ( pos + 1, dir - pos - 1 ) + "_" + str_proc_id.str ( ) + ".vtu" );
                pumesh->LinkEndChild ( piece );
                str_proc_id.str ( "" );
                str_proc_id.clear ( );
            }

            std::string str_filename = ( path + filename_without_suffix + ".pvtu" );
            FILE * pFile;
            pFile = fopen ( str_filename.c_str ( ), "w" );
            if ( pFile != NULL )
            {
                doc.SaveFile ( pFile );
                fclose ( pFile );
            }
            else
            {
                std::stringstream err;
                err << "Path to write the files (" << str_filename << ") does not exist!";
                LOG_ERROR ( err.str ( ) );
                throw std::runtime_error ( err.str ( ) );
            }
        }
    }

    template class ParallelFacetVisualization<double>;
    template class ParallelFacetVisualization<float>;
}
