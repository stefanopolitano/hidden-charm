# Python script to run the analysis of the hidden charm study
import os
import argparse
import yaml

def run_analysis(config_path):
    # Load configuration
    with open(config_path, "r") as config_file:
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
        os.system(f"python src/download.py {config_path}")

    if do_plot:
        print("Generating projections...")
        os.system(f"python src/project_sparse.py {config_path}")

    if do_fit:
        print("Fitting model...")
        os.system(f"python src/fit.py {config_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Arguments')
    parser.add_argument('config', type=str, help='Path to the configuration file')
    args = parser.parse_args()
    run_analysis(args.config)