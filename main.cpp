#include <boost/filesystem.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

#include <array>
#include <iostream>

//#include <CImg.h>
//
//using namespace cimg_library_suffixed;
//
//CImg<float> ph_dct_matrix(const int N)
//{
//    CImg<float> matrix = CImg<float>(N,N,1,1,1/sqrt((float)N));
//    const float c1 = sqrt(2.0f/N); 
//    for (int x=0;x<N;x++){
//        for (int y=1;y<N;y++){
//            *matrix.data(x,y) = c1*(float)cos((cimg::PI/2/N)*y*(2*x+1));
//        }
//    }
//    return matrix;
//}
//
//uint64_t ph_dct_imagehash( const CImg<uint8_t>& image)
//{
//    CImg<uint8_t> src = image;
//
//    CImg<float> meanfilter(7,7,1,1,1);
//    CImg<float> img;
//    if (src.spectrum() == 3){
//        img = src.RGBtoYCbCr().channel(0).get_convolve(meanfilter);
//    } else if (src.spectrum() == 4){
//        int width = img.width();
//        int height = img.height();
//        int depth = img.depth();
//        img = src.crop(0,0,0,0,width-1,height-1,depth-1,2).RGBtoYCbCr().channel(0).get_convolve(meanfilter);
//    } else {
//        img = src.channel(0).get_convolve(meanfilter);
//    }
//
//    img.resize(32,32);
//    const CImg<float> C  = ph_dct_matrix(32);
//    const CImg<float> Ctransp = C.get_transpose();
//
//    CImg<float> dctImage = (*C)*img*Ctransp;
//
//    CImg<float> subsec = dctImage.crop(1,1,8,8).unroll('x');;
//
//    float median = subsec.median();
//    uint64_t one = 0x0000000000000001;
//    uint64_t hash = 0x0000000000000000;
//    for (int i=0;i< 64;i++){
//        float current = subsec(i);
//        if (current > median)
//            hash |= one;
//        one = one << 1;
//    }
//
//    return hash;
//}


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
template < typename OnImageFile >
void search_windows( const boost::filesystem::path& path, OnImageFile on_image_file )
{

    std::array<wchar_t*, 2> extensions = { L".jpg", L".jpeg" };
    for (const auto extension : extensions)
    {
        WIN32_FIND_DATAW find_file_data;
        HANDLE find_file_handle = FindFirstFileW((path.wstring() + L"\\*" + extension).c_str(), &find_file_data);

        bool found_files = find_file_handle == INVALID_HANDLE_VALUE;
        while (found_files)
        {
            on_image_file(boost::filesystem::path(find_file_data.cFileName));

            found_files = FindNextFileW(find_file_handle, &find_file_data);
        }

        FindClose(find_file_handle);
    }
}


#endif

int main ( int argc, char* argv[])
{

    const std::wstring target_path(L"C:\\Users\\kettlitz\\Pictures");

    auto&& on_image_file = [] (const boost::filesystem::path& path) {std::wcout << path << std::endl;};

#ifdef _WIN32
    search_windows( target_path, on_image_file );
#else
    search_recursive( target_path, on_image_file );
#endif


    return 0;
}
