import requests # Import the requests library for making HTTP requests
import os       # Import the os module for path manipulation

def download_content(url, file_path=None):
    """
    Downloads content from a given URL. If a file_path is provided, it saves
    the content to that file; otherwise, it returns the content as a string.

    Args:
        url (str): The URL of the content to download.
        file_path (str, optional): The path to the file where the content should be saved.
                                   If None, the content is returned as a string.

    Returns:
        str or None: The content of the URL if file_path is None and successful,
                     otherwise None on success (content saved to file), or an
                     error message string on failure.
    """
    try:
        # Send an HTTP GET request to the specified URL.
        response = requests.get(url)

        # Raise an HTTPError for bad responses (4xx or 5xx).
        response.raise_for_status()

        if file_path:
            # If a file_path is provided, open the file in write binary mode
            # and write the content directly. Using 'wb' is safer for general
            # content as it handles binary data correctly.
            # .content returns the response body as bytes.
            with open(file_path, 'wb') as f:
                f.write(response.content)
            print(f"Content successfully saved to: {file_path}")
            return None # Indicate success by returning None
        else:
            # If no file_path, return the content as a string.
            # .text decodes the content based on the response's character encoding.
            return response.text

    except requests.exceptions.HTTPError as http_err:
        # Handle HTTP errors (e.g., 404 Not Found, 500 Internal Server Error)
        return f"HTTP error occurred: {http_err}"
    except requests.exceptions.ConnectionError as conn_err:
        # Handle network-related errors (e.g., DNS failure, refused connection)
        return f"Connection error occurred: {conn_err}"
    except requests.exceptions.Timeout as timeout_err:
        # Handle request timeout errors
        return f"Timeout error occurred: {timeout_err}"
    except requests.exceptions.RequestException as req_err:
        # Handle any other general requests exception
        return f"An unexpected error occurred: {req_err}"
    except IOError as io_err:
        # Handle errors related to file operations
        return f"File I/O error occurred: {io_err}"

if __name__ == "__main__":
    # The URL to download content from.
    url_to_download = "https://cedric.cnam.fr/~porumbed/graphs/C4000.5.col"
    # Define the file name to save the content.
    # We extract the base name from the URL for a sensible default.
    file_name = os.path.basename(url_to_download)
    # Construct the full file path in the current directory.
    output_file_path = os.path.join(os.getcwd(), file_name)

    print(f"Attempting to download content from: {url_to_download}")
    print(f"Saving content to: {output_file_path}")

    # Call the function to download and save the content.
    result = download_content(url_to_download, output_file_path)

    # Check the result of the download operation.
    if result is None:
        print("Download and save operation completed successfully.")
    else:
        # If result is not None, it means an error occurred.
        print(f"Download failed: {result}")

    # You can optionally read and print the content from the file
    # to verify, but it's not strictly necessary for the download task.
    # try:
    #     with open(output_file_path, 'r') as f:
    #         print("\n--- Content read from file ---")
    #         print(f.read())
    #         print("------------------------------")
    # except FileNotFoundError:
    #     print(f"File not found: {output_file_path}")
    # except Exception as e:
    #     print(f"Error reading file: {e}")
