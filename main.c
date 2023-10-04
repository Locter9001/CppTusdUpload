#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

struct upload_status {
    size_t uploaded_size;
};

size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
    struct upload_status *upload_ctx = (struct upload_status *) userp;
    const char *data = ((const char *) userp) + upload_ctx->uploaded_size;
    size_t remaining_bytes = strlen(data) - upload_ctx->uploaded_size;
    size_t buffer_size = size * nmemb;

    if (buffer_size < 1)
        return 0;

    if (buffer_size < remaining_bytes) {
        memcpy(ptr, data, buffer_size);
        upload_ctx->uploaded_size += buffer_size;
        return buffer_size;
    } else {
        memcpy(ptr, data, remaining_bytes);
        upload_ctx->uploaded_size += remaining_bytes;
        return remaining_bytes;
    }
}

char* base64(const char* input, int length)
{
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, input, length);
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    char* base64Data = (*bufferPtr).data;
    base64Data[(*bufferPtr).length] = '\0';

    return base64Data;
}

int tus_upload(char *url, char *file_path, char *file_id, char *file_name, char *authorization) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    struct upload_status upload_ctx = {0};

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    curl = curl_easy_init();
    if (curl) {
        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Set HTTP method to PATCH
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        char authorization_header[512];
        snprintf(authorization_header, sizeof(authorization_header), "Authorization: Bearer %s", authorization);
        headers = curl_slist_append(headers, authorization_header);

        // Set tus headers
        char content_type_header[256];
        snprintf(content_type_header, sizeof(content_type_header), "Content-Type: application/offset+octet-stream");
        headers = curl_slist_append(headers, content_type_header);

        char content_length_header[256];
        snprintf(content_length_header, sizeof(content_length_header), "Content-Length: %zu", file_size);
        headers = curl_slist_append(headers, content_length_header);

        char upload_length_header[256];
        snprintf(upload_length_header, sizeof(upload_length_header), "Upload-Length: %zu", file_size);
        headers = curl_slist_append(headers, upload_length_header);

        char upload_offset_header[256];
        snprintf(upload_offset_header, sizeof(upload_offset_header), "Upload-Offset: %zu", upload_ctx.uploaded_size);
        headers = curl_slist_append(headers, upload_offset_header);

        char tus_resumable_header[256];
        snprintf(tus_resumable_header, sizeof(tus_resumable_header), "Tus-Resumable: 1.0.0");
        headers = curl_slist_append(headers, tus_resumable_header);

        char *base64_file_id;
        base64_file_id = base64(file_id, (int)strlen(file_id));
        char *base64_file_name;
        base64_file_name = base64(file_name, (int)strlen(file_name));

        char metadata_header[256];
        snprintf(metadata_header, sizeof(metadata_header), "Upload-Metadata: id %s, filename %s", base64_file_id, base64_file_name);
        headers = curl_slist_append(headers, metadata_header);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set callback and data to read from
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        free(base64_file_id);
        free(base64_file_name);
        fprintf(stdout, "%d", res);
    }

    fclose(file);

    return 0;
}

int main() {
    char *url = "http://localhost:1081/files/";
    char *file_path = "F:/Locter/Videos/file.mp4";
    char *authorization = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyIjp7IklEIjoxLCJXaG8iOjEsIkRldmljZU1BQyI6IjAwLUUwLTEwLUVBLUI5LTAxIn0sImlzcyI6InR1c2QtYXBpIiwiZXhwIjoxNjk2MzgwODg3LCJuYmYiOjE2OTYzNzc4ODcsImlhdCI6MTY5NjM3Nzg4N30._vuoY_veCECrqNz2n_3KE2ZmQndoVT-YSy8XkLGei3c";
    char *file_name = "file.mp4";
    char *file_id = "1";

    tus_upload(url, file_path, file_id, file_name, authorization);
}