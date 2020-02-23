#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include <openssl/md5.h>

#include <cmdparser.hpp>

#include <array>
#include <iostream>
#include <sstream>
#include <future>

#include <turbojpeg.h>

#include <sqlite3.h>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Column.h>

#define cimg_use_jpeg
//#include <CImg.h>

template <typename OnImageFile>
void search_recursive(const boost::filesystem::path &dir, OnImageFile on_image_file)
{
    using namespace boost::filesystem;

    directory_iterator end_itr; // Default ctor yields past-the-end
    for (directory_iterator dir_iter(dir); dir_iter != end_itr; ++dir_iter)
    {
        const file_status &status = dir_iter->status();
        const path &file = dir_iter->path();

        if (is_directory(status))
        {
            try
            {
                search_recursive(file, on_image_file);
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }

            continue;
        }

        // Skip if not a file
        if (!is_regular_file(status))
        {
            continue;
        }

        const path &extension = file.extension();
        if (extension.compare(".jpg") == 0 || extension.compare(".jpeg") == 0)
        {
            on_image_file(file);
        }
    }
}

std::string
calc_hash(const std::vector<uint8_t> &file_buffer)
{
    uint8_t md5_buffer[MD5_DIGEST_LENGTH + 1];
    md5_buffer[0] = '\0';
    MD5(file_buffer.data(), file_buffer.size(), md5_buffer);

    std::stringstream stream;
    for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i)
    {
        stream << std::hex << static_cast<int>(md5_buffer[i]);
    }
    return stream.str();
}

/**
 * Image entry in the database
 */
struct ImageEntry
{
    std::string filename;
    std::string md5Hash;
    std::time_t lastWriteTime;
};

std::string formatTime(std::time_t time)
{
    tm utcTime;
    gmtime_r(&time, &utcTime);
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", &utcTime);
    return std::string(buffer);
}

ImageEntry
compute_image_hash(const boost::filesystem::path &path, bool verbose)
{
    std::vector<uint8_t> file_buffer;

    const std::time_t lastWriteTime = boost::filesystem::last_write_time(path);

    std::ifstream file;
    file.open(path.c_str(), std::ios::binary);

    if (!file.is_open())
    {
        std::wcerr << "Could not open " << path;
        return {};
    }

    file.seekg(0, std::ios::end);
    const auto file_size = file.tellg();

    file_buffer.resize(file_size);

    file.seekg(std::ios::beg);
    if (!file.read(reinterpret_cast<char *>(file_buffer.data()), file_buffer.size()))
    {
        std::wcerr << "Could not read " << path;
        file_buffer.clear();
        return {};
    }

    file.close();

    auto hash = calc_hash(file_buffer);

    if (verbose)
    {
        const std::string t = formatTime(lastWriteTime);
        std::cout << path.string() << " md5=" << hash << " time=" << t << std::endl;
    }

    return {.filename = path.string(), .md5Hash = std::move(hash), .lastWriteTime = lastWriteTime};
}

// Code to compute the image hash opposed to the file hash
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

void createImageTable(SQLite::Database &db)
{
    try
    {
        db.exec("CREATE TABLE images (id INTEGER PRIMARY KEY, filename TEXT, time TEXT, fileHash TEXT)");
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void addImageToTable(SQLite::Database &db, const ImageEntry &image)
{
    std::stringstream insertCmd;
    insertCmd << "INSERT INTO images VALUES (";
    insertCmd << "NULL, ";
    insertCmd << "\"" << image.filename << "\", ";
    insertCmd << "\"" << formatTime(image.lastWriteTime) << "\", ";
    insertCmd << "\"" << image.md5Hash << "\"";
    insertCmd << ")";

    try
    {
        int nb = db.exec(insertCmd.str());
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void updateDB(bool rescan, bool verbose, const boost::filesystem::path &imageFolder)
{
    using namespace boost::filesystem;
    if (!is_directory(imageFolder))
    {
        std::cerr << imageFolder.string() << " is not a directory." << std::endl;
        exit(1);
    }

    SQLite::Database db("imgdb.sqlite", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    if (!rescan && !db.tableExists("images"))
    {
        std::clog << "Database does not contain images... creating it." << std::endl;
        rescan = true;
    }

    if (rescan)
    {
        db.exec("DROP TABLE IF EXISTS images");
        createImageTable(db);

        std::cout << "Scanning " << imageFolder.string() << std::endl;

        std::vector<std::future<ImageEntry>> handles;
        auto &&on_image_file = [&handles, verbose](const path &path) {
            auto handle = std::async(std::launch::async, compute_image_hash, path, verbose);

            handles.emplace_back(std::move(handle));
        };

        search_recursive(imageFolder, on_image_file);

        size_t doneCount = 0;
        for (auto &handle : handles)
        {
            auto img = handle.get();

            if (img.filename.empty() || img.md5Hash.empty())
            {
                std::cerr << "Invalid image entry" << std::endl;
                continue;
            }

            addImageToTable(db, img);

            ++doneCount;
            if (!verbose)
            {
                std::cout << static_cast<size_t>(100.0 * doneCount / handles.size() + 0.5) << "%" << std::endl;
            }
        }
        std::cout << "done." << std::endl;
    }
}

int main(int argc, char *argv[])
{
    cli::Parser parser(argc, argv);

    const char *user = std::getenv("USER");
    if (user == nullptr)
    {
        std::cerr << "$USER not set. Cannot deduce user directory." << std::endl;
    }

    using boost::filesystem::path;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    const path imageFolder = path(L"C:\\Users\\").concat(user).concat(L"\\Pictures");
#elif __APPLE__
    const path imageFolder = path("/Users/").concat(user).concat("/Pictures");
#else
    // We assume everything else is Linux
    const path imageFolder = path("/home/").concat(user).concat("/Pictures");
#endif

    bool rescan = false;
    bool verbose = false;

    parser.set_optional<std::string>("f", "filename", "imgdb.sqlite", "Database filename");
    parser.set_callback<bool>("r", "rescan", [&rescan](cli::CallbackArgs &args) -> bool { rescan = true; }, "Rescan whole image folder and recreate database");
    parser.set_callback<bool>("v", "verbose", [&verbose](cli::CallbackArgs &args) -> bool { verbose = true; }, "Print log messages");
    parser.set_optional<std::string>("i", "input", imageFolder.string(), "Image folder.");

    parser.run_and_exit_if_error();

    updateDB(rescan, verbose, imageFolder);
}
