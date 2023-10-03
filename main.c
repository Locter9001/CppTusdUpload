#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

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
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");

        // Set Authorization header
        headers = curl_slist_append(headers, authorization);

        // Set tus headers
        char content_type_header[256];
        snprintf(content_type_header, sizeof(content_type_header), "Content-Type: application/offset+octet-stream");
        headers = curl_slist_append(headers, content_type_header);

        char upload_offset_header[256];
        snprintf(upload_offset_header, sizeof(upload_offset_header), "Upload-Offset: %zu", upload_ctx.uploaded_size);
        headers = curl_slist_append(headers, upload_offset_header);

        char tus_resumable_header[256];
        snprintf(tus_resumable_header, sizeof(tus_resumable_header), "Tus-Resumable: 1.0.0");
        headers = curl_slist_append(headers, tus_resumable_header);

        char metadata_header[256];
        snprintf(metadata_header, sizeof(metadata_header), "Upload-Metadata: id %s, filename %s", file_id, file_name);
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


        fprintf(stdout, "upload is ok!");
    }

    fclose(file);

    return 0;
}

int main() {
    char *url = "http://your-tus-server/files/";
    char *file_path = "";
    char *authorization = "";
    char *file_name = "";
    char *file_id = "";

    tus_upload(url, file_path, file_id, file_name, authorization);
}