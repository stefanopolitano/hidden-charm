# Python script to run the analysis of the hidden charm study
import os
import subprocess
import yaml

def run_analysis():
    # Load configuration
    with open("config/config_etac.yml", "r") as config_file:
        config = yaml.safe_load(config_file)

    # Prepare output directory
    output_dir = config.get("outdir", "results/")
    # Create the output directory if it doesn't exist otherwise, if it exists, overwrite the existing results by removing the directory and creating a new one
    if os.path.exists(output_dir):
        os.system(f"rm -rf {output_dir}")
    os.makedirs(output_dir, exist_ok=True)

    # Run the analysis script
    do_download = config.get("do_download", False)
    do_plot = config.get("do_projection", False)
    do_fit = config.get("do_fit", False)

    if do_download:
        print("Downloading data...")
        os.system("python src/download.py")

    if do_plot:
        print("Generating projections...")
        os.system("python src/project_sparse.py")

    if do_fit:
        print("Fitting model...")
        os.system("python src/fit.py")

if __name__ == "__main__":
    run_analysis()