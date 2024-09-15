/*
    Muhammad Rehan
    22P-9106 | BSE-5A | CS2006 Operating Systems
    Assignment 01 | Question no 03

*/
#include <iostream>
// for file streams
#include <fstream>
// for the string & wstring
#include <string>
// for the regular expressions
#include <regex>
// for file system operations
#include <filesystem>
// for smart pointers
#include <memory>
// for dynamic arrays
#include <vector>
// for exception handling
#include <exception>
// for localization  of the Unicode
#include <locale>
// for the string conversions
#include <codecvt>
// for memset
#include <cstring>
// for fork(), pipe(), read(), write()
#include <unistd.h>
// for pid_t
#include <sys/types.h>
// for waitpid()
#include <sys/wait.h>

using namespace std;

/*
 *  Here we have created a custom class
 *  inheriting from execution clas of C++,
 * */
class SecureTransformException : public exception {
public:
    /*
     * The constructor of our class, takes am error code
     * and a message and assign the values
     *          int code is the error code for the exception.
     *          string message is the error message
     *          describing exception.
     *          The move function is used to transfer ownership
     *          of error message string, and it helps avoids
     *          unnecessary copying (so no cloning)
     * */
    SecureTransformException(int code, string message) : errorCode(code), errorMsg(move(message)) {}

    /*
     *  overriding what function from the exception class,
     *  returning the const char ptr, the const noexcept
     *  override helps without modifying the object, and noexcept
     *  means this function won't throw an exception itself
     *      and the errorMsg.c_str() is converting
     *      string errorMsg to null-terminated C string.
     * */
    [[nodiscard]] const char *what() const noexcept override { return errorMsg.c_str(); }

    /*
     *  this method return the error code related to our
     *  exception, and same use of the const and noexcept
     * */
    [[nodiscard]] int code() const noexcept { return errorCode; }

private:
    int errorCode;
    string errorMsg;
};

/*
 *  Using an enum of a set of constants to represent
 *  different types of errors that could occur in our
 *  code, these are predefined
 * */
enum ErrorCode {
    SUCCESS = 0,
    INVALID_INPUT = 1,
    FILE_NOT_FOUND = 2,
    WRITE_ERROR = 4,
    TRANSFORMATION_ERROR = 5,
    PROCESS_ERROR = 6,
    UNKNOWN_ERROR = -1
};

// Enum for transformation choice of the user
enum class TransformationType {
    DECRYPT = 1,
    REDACT = 2
};

// functions and class prototypes
void runSecureTransform();


/*
 *  So this class is for data transformation
 *  all of its implementation is commented below
 * */
class DataTransformer {
public:
    explicit DataTransformer(TransformationType type);

    wstring transformData(const wstring &data);

private:
    TransformationType transformationType;

    static wstring decryptData(const wstring &data);

    static wstring redactData(const wstring &data);
};

void zeroMemory(volatile wchar_t *buffer, size_t size);

int main() {
    try {
        runSecureTransform();
    } catch (const SecureTransformException &ex) {
        cerr << "Error [" << ex.code() << "]: " << ex.what() << endl;
        return ex.code();
    } catch (const exception &ex) {
        cerr << "Unhandled exception: " << ex.what() << endl;
        return ErrorCode::UNKNOWN_ERROR;
    }

    return ErrorCode::SUCCESS;
}

/*
 *  this function will manage the whole reading input
 *  and calling a transformation method, processing the
 *  file and then converting it into chucks (if the file is
 *  too large), then transforming the data, writing it to
 *  the pipe and using the child process for the data trans-
 *  -formation.
 *
 * */

void runSecureTransform() {

    /*
     *  Setting the global locale of our
     *  code to my own machine Unicode chars
     *  around the all input and standard operations
     * */
    locale::global(locale(""));

    /*
     *   let's first mention it, that i am using
     *   wide chars throughout the code, to support
     *   all Unicode chars
     *
     */

    wcout << L"Enter the input file name: ";
    wstring inputFileName;
    getline(wcin, inputFileName);
    wcout << L"Enter the output file name: ";
    wstring outputFileName;
    getline(wcin, outputFileName);

    // Here I am validating the file names given by the user
    if (inputFileName.empty() || outputFileName.empty()) {
        // throwing the exception if any of the file name is empty
        throw SecureTransformException(ErrorCode::INVALID_INPUT, "File names cannot be empty.");
    }

    // here user is selecting a transformation method
    wcout << L"Choose a transformation method:" << endl;
    wcout << L"1. Decrypt Data" << endl;
    wcout << L"2. Redact Sensitive Information" << endl;
    wcout << L"Enter your choice (1 or 2): ";
    int choice = 0;
    wcin >> choice;

    // wcin.ignore clearing the input buffer
    wcin.ignore(numeric_limits<streamsize>::max(), '\n');

    /*
     * here we are validating the transformation choice,
     * by type casting the enums to ints to check them against
     * the choice
     * */
    if (choice != static_cast<int>(TransformationType::DECRYPT) &&
        choice != static_cast<int>(TransformationType::REDACT)) {
        // throwing the exception if choice isn't correct.
        throw SecureTransformException(ErrorCode::INVALID_INPUT, "Invalid transformation choice.");
    }
    /*
     *  here we are type casting the choice into the enum type
     *  using auto so it could get the type itself.
     * */
    auto transformationType = static_cast<TransformationType>(choice);

    /*
     * here I am converting file names to filesystem::path
     * from the wstring, filesystem::path is the part of
     * c++ 17 version, it helps into managing file paths
     * */
    filesystem::path inputFilePath = inputFileName;
    filesystem::path outputFilePath = outputFileName;

    /*
     * Creating a pipe for interprocess communication
     * pipefd[0] is for the read end,
     * pipefd[1] is for the write end.
     * */
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        // throwing the exception if couldn't create the pipe.
        throw SecureTransformException(ErrorCode::PROCESS_ERROR, "Failed to create pipe.");
    }

    // here creating a child process
    pid_t pid = fork();

    if (pid == -1) {
        // if the fork failed then close the pipe
        close(pipefd[0]);
        close(pipefd[1]);
        // throwing the exception if couldn't create the child process.
        throw SecureTransformException(ErrorCode::PROCESS_ERROR, "Failed to fork a child process.");
    } else if (pid == 0) {
        // and if we are able to create a fork then close the read end
        close(pipefd[0]);
        try {
            /*
             * here we are opening the input file securely,
             * using the wide char system wifstream
             * */
            wifstream inputFile(inputFilePath, ios::binary);
            if (!inputFile) {
                // throwing the exception if couldn't open the file
                throw SecureTransformException(ErrorCode::FILE_NOT_FOUND, "Input file does not exist.");
            }

            // Setting  the locale for wide chars on the input file
            inputFile.imbue(locale(""));

            /*
             * using the constexpr to get the value of the
             * var at the compile time, and making a buffer
             * of a 8192 size and making a vector to store
             * the chucks of the data read from the file
             * */
            constexpr size_t bufferSize = 8192;
            vector<wchar_t> readBuffer(bufferSize);

            // Now lets make an object of the DataTransformer
            DataTransformer transformer(transformationType);

            /*
             *  now make a wstring convert with codecvt_utf8<wchar_t> to
             *  convert wide strings to utf-8 strings for writing
             *  to our pipe.
             * */
            wstring_convert<codecvt_utf8<wchar_t>> converter;


            /*
             * this while loop is reading data from the
             * input file into chucks, inputfile.read(),
             * trys to read the bufferSize chars, from input
             * file to the readBuffer
             *
             * and the inputFile.gcount() return the numbers of
             * the chars were got from the file. if the read()
             * returns false then it means we got to the end of the
             * file, so the gcount ensures that we read the every
             * last chuck of the data
             * */
            while (inputFile.read(readBuffer.data(), static_cast<streamsize>(readBuffer.size())) ||
                   inputFile.gcount() > 0) {

                /*
                 * then the inputFile.gcount is type castes
                 * into the size_t to store the numbers of chars
                 * */

                size_t count = static_cast<size_t>(inputFile.gcount());

                /*
                 * dataChuck is made as a wide string from the 1st
                 * count chars in the readBuffer
                 * */
                wstring dataChunk(readBuffer.data(), count);

                /*
                 * Now here we are calling the transformData() function
                 * to the process the data as per the Decrypt or Redact.
                 * */
                wstring transformedChunk = transformer.transformData(dataChunk);

                /*
                 * Here converting transformed data
                 * to utf-8 for writing to pipe
                 *
                 *  that object we created above
                 *  (wstring_convert<codecvt_utf8<wchar_t>> converter)
                 *  is converting the wide-char string into utf-8 encoded
                 *  string, as pipe only works with byte streams
                 *
                 * */
                string transformedChunkUTF8 = converter.to_bytes(transformedChunk);

                /*
                 *  Now lets write the data to the pipe using the write
                 *  system call, transformedChunkUTF8.c_str() is a pointer
                 *  to the raw char array (byte representation of the utf-8
                 *  string), and the transformedChunkUTF8.size() giving the
                 *  number of bytes to be written in our pipe.

                 * */
                if (write(pipefd[1], transformedChunkUTF8.c_str(), transformedChunkUTF8.size()) == -1) {
                    // throwing the exception if couldn't write to pipe
                    throw SecureTransformException(ErrorCode::PROCESS_ERROR, "Failed to write to pipe.");
                }

                /*
                 *  after we have sent the data to the pipe,
                 *  now lets make sure we ain't got any sensitive
                 *  data in the memory,
                 *
                 *  So we are using the zeroMemory to overwrite
                 *  the whole data with zero
                 *
                 * */
                zeroMemory(&dataChunk[0], dataChunk.size());
                zeroMemory(&transformedChunk[0], transformedChunk.size());

                /*
                 *  same here, as above, overwriting the data
                 *  with zeros making sure the raw byte data
                 *  is removed securely.
                 * */
                memset(&transformedChunkUTF8[0], 0, transformedChunkUTF8.size());
            }

            // closing the input file
            inputFile.close();

            // Same practice of making the whole buffer zero.
            zeroMemory(&readBuffer[0], readBuffer.size());

        } catch (const SecureTransformException &ex) {
            /*
             *  this catch block is here to throw any
             *  exception if we messed up in the code
             *  above (try block)
             *
             *         SecureTransformException &ex
             *         is to throw particular exception
             *         as per the error
             *
             *         and here error code from the
             *         exception is stored into
             *         int errorCode = ex.code()
             *
             * */
            int errorCode = ex.code();
            write(pipefd[1], &errorCode, sizeof(errorCode));
            close(pipefd[1]);
            exit(EXIT_FAILURE);
        }


        /*
         * if no exception was thrown then close the
         * write end and just exit the child with its
         * status
         * */
        close(pipefd[1]);
        exit(EXIT_SUCCESS);
    } else {
        // Here is our parent process

        // Closing its unused write end
        close(pipefd[1]);

        // Here we are just waiting for the child process to complete
        int status;
        /*
         *  so now if we get an error while waiting for the
         *  child, then we need to throw an exception,
         * */
        if (waitpid(pid, &status, 0) == -1) {

            /*
             *  in that case just close the pipe read end
             *  and throw an exception.
             * */
            close(pipefd[0]);
            throw SecureTransformException(ErrorCode::PROCESS_ERROR, "Failed to wait for child process.");
        }

        /*
         *  using the WIFEXITED here, so iff the child process didn't
         *  exit successfully then close the read end of the pipe
         *
         *  and using WIFSIGNALED() if the child was terminated by
         *  the signal, then again then closing the read end of the pipe
         *  and throwing an exception.
         * */
        if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
            close(pipefd[0]);
            throw SecureTransformException(ErrorCode::PROCESS_ERROR, "Child process terminated with an error.");
        } else if (WIFSIGNALED(status)) {
            close(pipefd[0]);
            throw SecureTransformException(ErrorCode::PROCESS_ERROR, "Child process was terminated by a signal.");
        }

        /*
         *  After making sure child survives, parent opens the output
         *  file where the data will be written getting from the pipe
         *
         *  a wide chara output file stream is opened wit the file path
         *  binary mode as the data is raw
         * */
        wofstream outputFile(outputFilePath, ios::binary);
        if (!outputFile) {
            // throwing the exception if couldn't open the output file
            close(pipefd[0]);
            throw SecureTransformException(ErrorCode::WRITE_ERROR, "Unable to open output file.");
        }

        // Again setting locale for wide characters
        outputFile.imbue(locale(""));


        /*
         * using the constexpr to get the value of the
         * var at the compile time, and making a buffer
         * of a 8192 size, which will store the data we
         * will get from the pipe.
         * */
        constexpr size_t bufferSize = 8192;
        char readBuffer[bufferSize];
        // this will be used to keep track of the bytes reading from
        // the pipe
        ssize_t bytesRead;

        /*
         *  now make a wstring convert with codecvt_utf8<wchar_t> from
         *  convert utf-8 encoded strings to wide strings to
         *  for writing to the output file
         * */
        wstring_convert<codecvt_utf8<wchar_t>> converter;

        /*
         *  this loop will read data from the read end of the
         *  pipe and into readBuffeer into chuck to the
         *  bufferSize bytes
         *      The loop will rin until read() returns a 0
         *      means the end of the pipe or neg value
         *      meaning am error and if bytesRead is
         *      greater than 0 it means there is data to process.
         * */
        while ((bytesRead = read(pipefd[0], readBuffer, sizeof(readBuffer))) > 0) {

            /*
             *  here we are converting our UTF-8 raw data
             *  to wide string, constructor of the converter
             *  uses bytesRead to check how much of the
             *  readBuffer to use.
             * */
            string utf8Data(readBuffer, static_cast<size_t>(bytesRead));

            /*
             *  wstring wideData converts utf-8 string (utf8Data)
             *  into wide-char string, the converter.from_bytes()
             *  handling the converting process here
             * */
            wstring wideData = converter.from_bytes(utf8Data);

            // here we are just writing the data to output file
            /*
             *  writing the wideData to the output file
             *  wideData.c_str() is ptr to chars in the
             *  wchar_t array, static_cast<streamsize>(wideData.size())
             *  is type casting the size of wideData
             *  to streamsize used by the write() function.
             * */

            outputFile.write(wideData.c_str(), static_cast<streamsize>(wideData.size()));

            /*
             *  Doing the same practice we used in the child process
             *  as now we are done with the writing lets overwriting
             *  readBuffer, wideDat and utf8Data with zero for security
             * */
            memset(readBuffer, 0, sizeof(readBuffer));
            zeroMemory(&wideData[0], wideData.size());
            memset(&utf8Data[0], 0, utf8Data.size());
        }
        /*  And if the bytesRead is -1 means
         *  we got an error while reading from the
         *  pipe, so close the read end of the pipe
         *  and just close the file and throw an
         *  exception
         *  */
        if (bytesRead == -1) {
            close(pipefd[0]);
            outputFile.close();
            throw SecureTransformException(ErrorCode::PROCESS_ERROR, "Failed to read from pipe.");
        }
        /*
         *   and if didn't get an error and then properly
         *   close the file and the read end of the pipe
         * */

        outputFile.close();
        close(pipefd[0]);

        // Here we are just generating the summary report
        wcout << L"\nTransformation complete." << endl;
        wcout << L"Summary Report:" << endl;
        wcout << L"Input File: " << inputFileName << endl;
        wcout << L"Output File: " << outputFileName << endl;
        if (transformationType == TransformationType::DECRYPT) {
            wcout << L"Transformation: Decryption (Reverse Caesar Cipher)" << endl;
        } else if (transformationType == TransformationType::REDACT) {
            wcout << L"Transformation: Redaction of Sensitive Information" << endl;
            wcout << L"Patterns Redacted: Email Addresses, Credit Card Numbers, SSNs" << endl;
        }
    }
}

/*
 * The transformData function a input wide string (wstring) and
 * do a transformation based on the object transformationType.
 *
 * iff the type is DECRYPT, it will call the decryptData function
 * and If the type is REDACT, it will calls the redactData function
 * to redact sensitive information from the data.
 *
 * If it gets a unknown transformation type is ,
 * it throws a SecureTransformException.
 *
 * */
DataTransformer::DataTransformer(TransformationType type) : transformationType(type) {}

wstring DataTransformer::transformData(const wstring &data) {
    switch (transformationType) {
        case TransformationType::DECRYPT:
            return decryptData(data);
        case TransformationType::REDACT:
            return redactData(data);
        default:
            throw SecureTransformException(ErrorCode::TRANSFORMATION_ERROR, "Unknown transformation type.");
    }
}

wstring DataTransformer::decryptData(const wstring &data) {
    // here we are creating a copy of the input data
    wstring decryptedData = data;

    for (wchar_t &c: decryptedData) {
        if (iswalpha(c))
            /*
             * basic logic to handle the
             * */

            // Handle alphabetic characters
            if (c == L'A') c = L'Z';   // Wrap-around for uppercase 'A'
            else if (c == L'a') c = L'z';  // Wrap-around for lowercase 'a'
            else c = c - 1;  // Shift character back by one
        } else if (iswdigit(c)) {
            // Handle numeric characters
            if (c == L'0') c = L'9';   // Wrap-around for '0'
            else c = c - 1;  // Shift digit back by one
        } else if (iswspace(c)) {
            // Keep spaces unchanged
            continue;
        } else if (iswprint(c)) {
            // Handle other printable characters
            c = c - 1;  // Shift character back by one
            if (!iswprint(c)) {
                c = L'~';  // In case of non-printable result, wrap around
            }
        }
    }
    return decryptedData;  // Return the decrypted data
}


wstring DataTransformer::redactData(const wstring &data) {
    wstring redactedData = data; // Create a copy of the input data

    // Define regular expressions for sensitive information patterns

    // Email address pattern (supports Unicode characters)
    wregex emailRegex(LR"(([\w.%+-]+)@([\w.-]+)\.([A-Za-z]{2,}))");

    // Credit card number pattern (supports various formats)
    wregex ccRegex(LR"(\b(?:\d[ -]*?){13,16}\b)");

    // Social Security Number pattern
    wregex ssnRegex(LR"(\b\d{3}[- ]?\d{2}[- ]?\d{4}\b)");

    // Replace email addresses with "[REDACTED]"
    redactedData = regex_replace(redactedData, emailRegex, L"[REDACTED]");

    // Replace credit card numbers with "[REDACTED]"
    redactedData = regex_replace(redactedData, ccRegex, L"[REDACTED]");

    // Replace SSNs with "[REDACTED]"
    redactedData = regex_replace(redactedData, ssnRegex, L"[REDACTED]");

    return redactedData; // Return the redacted data
}

/*
 *  the function we are using above to remove the
 *  data, here we are just taking the buffer, its
 *  size and until we get to the end of it, we just
 *  keep setting each element of the buffer to 0
 * */
void zeroMemory(volatile wchar_t *buffer, size_t size) {
    while (size--) {
        *buffer++ = 0;
    }
}
