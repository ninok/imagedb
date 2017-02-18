#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

#include <array>
#include <iostream>

#include <turbojpeg.h>

#define cimg_use_jpeg
#include <CImg.h>

template < typename OnImageFile >
void search_recursive( const boost::filesystem::path& path, OnImageFile on_image_file )
{
    using namespace boost::filesystem;

    directory_iterator end_itr; // Default ctor yields past-the-end
    for (directory_iterator dir_iter( path ); dir_iter != end_itr; ++dir_iter)
    {
        if ( is_directory(dir_iter->status() ) )
        {
            search_recursive( dir_iter->path(), on_image_file );
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

#ifdef _WIN32

#include <stdio.h>
#include <windows.h>
#include <Wincrypt.h>

#define BUFSIZE 1024
#define MD5LEN  16

boost::optional< std::string >
calc_hash( const boost::filesystem::path& filename )
{
    DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = NULL;
    BYTE rgbFile[BUFSIZE];
    DWORD cbRead = 0;
    BYTE rgbHash[MD5LEN];
    DWORD cbHash = 0;
    CHAR rgbDigits[] = "0123456789abcdef";
    //LPCWSTR filename=L"filename.txt";
    // Logic to check usage goes here.

    hFile = CreateFileW(filename.c_str( ),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwStatus = GetLastError();
        std::wcerr << "Error opening file "<< filename <<" Error: " << dwStatus;
        return boost::none;
    }

    // Get handle to the crypto provider
    if (!CryptAcquireContext(&hProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        std::wcerr << "CryptAcquireContext failed: " << dwStatus;
        CloseHandle(hFile);
        return boost::none;
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        dwStatus = GetLastError();
        std::wcerr << "CryptCreateHash failed: " << dwStatus;
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        return boost::none;
    }

    while (bResult = ReadFile(hFile, rgbFile, BUFSIZE, 
        &cbRead, NULL))
    {
        if (0 == cbRead)
        {
            break;
        }

        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            printf("CryptHashData failed: %d\n", dwStatus); 
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return boost::none;
        }
    }

    if (!bResult)
    {
        dwStatus = GetLastError();
        printf("ReadFile failed: %d\n", dwStatus); 
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        return boost::none;
    }

    cbHash = MD5LEN;
    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        dwStatus = GetLastError();
        std::wcerr << "CryptGetHashParam failed: " << dwStatus;
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);

        return boost::none;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    return std::string( reinterpret_cast<const char*>( rgbHash ), cbHash );
}
#endif

struct RGBX
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t x;
};


int main ( int argc, char* argv[])
{
    using namespace boost::filesystem;
    using namespace cimg_library;

    const std::wstring target_path(L"C:\\Users\\kettlitz\\Pictures");

    std::vector< boost::filesystem::path > files;

    auto&& on_image_file = [&files] (const path& path) { files.push_back( path ); };

    search_recursive( target_path, on_image_file );

    std :: wcout << "Found " << files.size( ) << " files" << std::endl;

    std::wcout << "Computing hashes ..." << std::endl;

    std::vector< uint8_t > file_buffer;
    std::vector< uint8_t > bitmap_buffer;

    std::vector< std::string > hashes;
    hashes.reserve( files.size( ) );

    //tjhandle jpeg_handle = tjInitDecompress( );
    //if ( !jpeg_handle )
    //{
    //    std::wcerr << "Could not init turbo jpeg decompression ";
    //    return -1;
    //}
    for ( const auto& filename : files )
    {
        //std::ifstream file;
        //file.open( filename.c_str( ), std::ios::binary );

        //if ( !file.is_open( ) )
        //{
        //    std::wcerr << "Could not open " << filename;
        //    continue;
        //}

        //file.seekg( 0, std::ios::end );
        //const auto file_size = file.tellg( );

        //file_buffer.resize( file_size );

        //file.seekg( std::ios::beg );
        //if ( !file.read( reinterpret_cast< char* >( file_buffer.data( ) ), file_buffer.size( ) ) )
        //{
        //    std::wcerr << "Could not read " << filename;
        //    continue;
        //}

        //file.close( );

        //int width;
        //int height;
        //tjDecompressHeader ( jpeg_handle, file_buffer.data( ), file_buffer.size( ), &width, &height );

        //int pitch = tjPixelSize[ TJPF_RGB ] * width;

        //bitmap_buffer.resize( pitch * height);

        //tjDecompress2( jpeg_handle,  file_buffer.data( ), file_buffer.size( ), bitmap_buffer.data( ), width, pitch, height, TJPF_RGB, 0 );

        //CImg< uint8_t > image( bitmap_buffer.data( ), width, height, 1, 3 );
        CImg< uint8_t > image;
        image.load_jpeg( filename.string().c_str( ) );
        image.display( );


        //const auto& hash = calc_hash( file );
        //if ( hash )
        //{
        //    hashes.emplace_back( std::move( *hash ) );
        //}
    }

    std::wcout << "done." << std::endl;
    return 0;
}
