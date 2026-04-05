# Code to download the AnalysisResults.root file from the CERN server with alien.py
import os
import argparse
import yaml

def download_data(config_path):
    # Load configuration
    with open(config_path, "r") as config_file:
        config = yaml.safe_load(config_file)

    # Prepare output directory
    output_dir = config.get("outdir", "results/")
    output_dir_download = os.path.join(output_dir, "downloaded_data")
    os.makedirs(output_dir_download, exist_ok=True)

    # Download the data using alien.py
    alien_path = config['download']['alien_path']
    alien_path = [path + "/AnalysisResults.root" for path in alien_path]  # Append the filename to each path
    for index, ifile in enumerate(alien_path):
        local_path = os.path.join(output_dir_download, f"AnalysisResults_{index}.root")
        print(f"Downloading {ifile} to {local_path}")
        os.system(f"alien_cp -T 10 {ifile} file:{local_path}")
        print(f"Downloaded {ifile} to {local_path}")
    
    # If multiple files were downloaded, merge them into a single file using ROOT's hadd command
    if len(alien_path) > 1:
        merged_path = os.path.join(output_dir_download, "Merged_AnalysisResults.root")
        os.system(f"hadd -f {merged_path} {os.path.join(output_dir_download, 'AnalysisResults_*.root')}")
        print(f"Merged files into {merged_path}")
        # Remove the individual files after merging
        os.system(f"rm {os.path.join(output_dir_download, 'AnalysisResults_*.root')}")
        print(f"Removed individual files after merging")
        # Rename the merged file to the final name
        os.system(f"mv {merged_path} {local_path}")
        print(f"Renamed merged file to {local_path}")
    
    # Rename the downloaded file to the final name
    outlabel_filename = f"AnalysisResults_{config['out_label']}.root"
    os.system(f"mv {local_path} {os.path.join(output_dir_download, outlabel_filename)}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Arguments')
    parser.add_argument('config', type=str, help='Path to the configuration file')
    args = parser.parse_args()
    download_data(args.config)
