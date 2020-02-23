#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include <openssl/md5.h>

#include <array>
#include <iostream>
#include <sstream>
#include <future>

#include <turbojpeg.h>

#include <SQLiteCpp/Database.h>
#include <sqlite3.h>

#define cimg_use_jpeg
//#include <CImg.h>

template < typename OnImageFile >
void search_recursive( const boost::filesystem::path& path, OnImageFile on_image_file )
{
    using namespace boost::filesystem;

    directory_iterator end_itr; // Default ctor yields past-the-end
    for (directory_iterator dir_iter( path ); dir_iter != end_itr; ++dir_iter)
    {
        if ( is_directory(dir_iter->status() ) )
        {
            try
            {
                search_recursive( dir_iter->path(), on_image_file );
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            
            continue;
        }

        // Skip if not a file
        if ( !is_regular_file(dir_iter->status() ) )
        {
            continue;
        }

        const boost::filesystem::path& extension = dir_iter->path().extension( );
        if ( !extension.compare( L".jpg" ) && !extension.compare( L".jpeg" ) )
        {
            continue;
        }

        on_image_file( dir_iter->path() );
    }
}

std::string
calc_hash( const std::vector<uint8_t>& file_buffer)
{
    uint8_t md5_buffer[MD5_DIGEST_LENGTH+1];
    md5_buffer[0] = '\0';
    MD5(file_buffer.data(), file_buffer.size(), md5_buffer);
    
    std::stringstream stream;
    for ( size_t i = 0; i < MD5_DIGEST_LENGTH; ++i)
    {
        stream << std::hex << static_cast<int>(md5_buffer[i]);
    }
    return stream.str();
}

std::string compute_image_hash( const boost::filesystem::path& path )
{
    std::vector< uint8_t > file_buffer;

    std::ifstream file;
    file.open(path.c_str(), std::ios::binary);

    if (!file.is_open())
    {
        std::wcerr << "Could not open " << path;
        return "";
    }

    file.seekg(0, std::ios::end);
    const auto file_size = file.tellg();

    file_buffer.resize(file_size);

    file.seekg(std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(file_buffer.data()), file_buffer.size()))
    {
        std::wcerr << "Could not read " << path;
        file_buffer.clear();
        return "";
    }

    file.close();

    auto hash = calc_hash( file_buffer);

    std::wcout << path.c_str() << L" md5 sum = " << hash.c_str() << std::endl;

    return std::move(hash);
}

int main ( int argc, char* argv[])
{
    using namespace boost::filesystem;

    try
    {
        SQLite::Database db( "imgdb.sqlite", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
        db.exec("DROP TABLE IF EXISTS images");
        db.exec("CREATE TABLE images (id INTEGER PRIMARY KEY, filename TEXT, md5hash TEXT)");
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    const std::wstring target_path(L"C:\\Users\\kettlitz\\Pictures");
#elif __APPLE__
    const std::wstring target_path(L"/Users/kettlitz/Pictures");
#else
    // We assume everything else is Linux
    const std::wstring target_path(L"/home/kettlitz/Pictures");
#endif

    std::vector< std::future<std::string> > handles;
    auto&& on_image_file = [&handles] (const path& path) {
        auto handle = std::async(std::launch::async, compute_image_hash, path);
        handles.emplace_back(std::move(handle));
    };

    // search_recursive( target_path, on_image_file );

    std::wcout << "Computing hashes ..." << std::endl;

    //std::vector< uint8_t > bitmap_buffer;

    //std::vector< std::string > hashes;
    //hashes.reserve( files.size( ) );

    //tjhandle jpeg_handle = tjInitDecompress( );
    //if ( !jpeg_handle )
    //{
    //    std::wcerr << "Could not init turbo jpeg decompression ";
    //    return -1;
    //}
    //for ( const auto& filename : files )
    //{
    //    int width;
    //    int height;
    //    tjDecompressHeader ( jpeg_handle, file_buffer.data( ), file_buffer.size( ), &width, &height );

    //    int pitch = tjPixelSize[ TJPF_RGB ] * width;

    //    bitmap_buffer.resize( pitch * height);

    //    tjDecompress2( jpeg_handle,  file_buffer.data( ), file_buffer.size( ), bitmap_buffer.data( ), width, pitch, height, TJPF_RGB, 0 );

    //    hashes.emplace_back( calc_hash( file_buffer ) );

    //    std::wcout << filename << L" md5 sum = " << hashes.back() << std::endl;
    //}


    for(auto& handle : handles)
    {
        auto hash = handle.get();
    }

    std::wcout << "done." << std::endl;
    return 0;
}
