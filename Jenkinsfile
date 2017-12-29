#!groovy

// Jenkinsfile for compiling, testing, and packaging Aseba.
// Requires CMake plugin from https://github.com/davidjsherman/aseba-jenkins.git in global library.

pipeline {
	agent any

	// Jenkins will prompt for parameters when a branch is build manually
	// but will use default parameters when the entire project is built.
	parameters {
		string(defaultValue: 'David Sherman/dashel/master', description: 'Dashel project', name: 'project_dashel')
		string(defaultValue: 'David Sherman/enki/master', description: 'Enki project', name: 'project_enki')
	}
	
	// Everything will be built in the build/ directory.
	// Everything will be installed in the dist/ directory.
	stages {
		stage('Prepare') {
			steps {
				echo "project_dashel=${env.project_dashel}"
				echo "project_enki=${env.project_enki}"

				// Jenkins will automatically check out the source
				// Fixme: Stashed source includes .git otherwise submodule update fails
				sh 'git submodule update --init'
				stash name: 'source'

				// Dashel and Enki are retrieved from archived artifacts
				script {
					def p = ['debian','macos','windows'].collectEntries{
						[ (it): {
								node(it) {
									unstash 'source'
									copyArtifacts projectName: "${env.project_dashel}",
										  filter: 'dist/'+it+'/**',
										  selector: lastSuccessful()
									copyArtifacts projectName: "${env.project_enki}",
										  filter: 'dist/'+it+'/**',
										  selector: lastSuccessful()
									stash includes: 'dist/**', name: 'dist-externals-' + it
								}
							}
						]
					}
					parallel p;
				}
			}
		}
		stage('Compile') {
			parallel {
				stage("Compile on debian") {
					agent {
						label 'debian'
					}
					steps {
						unstash 'source'
						unstash 'dist-externals-debian'
						CMake([label: 'debian'])
						stash includes: 'dist/**', name: 'dist-aseba-debian'
						stash includes: 'build/**', name: 'build-aseba-debian'
					}
				}
				stage("Compile on macos") {
					agent {
						label 'macos'
					}
					steps {
						unstash 'source'
						unstash 'dist-externals-macos'
						script {
							env.macos_enki_DIR = sh ( script: 'dirname $(find $PWD/dist/macos -name enkiConfig.cmake | head -1)', returnStdout: true).trim()
						}
						echo "macos_enki_DIR=${env.macos_enki_DIR}"
						unstash 'source'
						CMake([label: 'macos',
							   envs: [ "enki_DIR=${env.macos_enki_DIR}" ] ])
						stash includes: 'dist/**', name: 'dist-aseba-macos'
					}
				}
				stage("Compile on windows") {
					agent {
						label 'windows'
					}
					steps {
						unstash 'source'
						unstash 'dist-externals-windows'
						CMake([label: 'windows'])
						stash includes: 'dist/**', name: 'dist-aseba-windows'
					}
				}
			}
		}
		stage('Smoke Test') {
			// Only do some tests. To do: add test labels in CMakeLists to distinguish between
			// obligatory smoke tests (to be done for every PR) and extended tests only for
			// releases or for end-to-end testing.
			parallel {
				stage("Smoke test on debian") {
					agent {
						label 'debian'
					}
					steps {
						unstash 'build-aseba-debian'
						dir('build/debian') {
							sh "LANG=en_US.UTF-8 ctest -E 'e2e.*|simulate.*|.*http.*|valgrind.*'"
						}
					}
				}
			}
		}
		stage('Extended Test') {
			// Extended tests are only run for the master branch.
			when {
				expression {
					return env.BRANCH == 'master'
				}
			}
			parallel {
				stage("Extended test on debian") {
					agent {
						label 'debian'
					}
					steps {
						unstash 'build-aseba-debian'
						dir('build/debian') {
							sh "LANG=en_US.UTF-8 ctest -R 'e2e.*|simulate.*|.*http.*|valgrind.*'"
						}
					}
				}
			}
		}
		// No stage('Package'), packaging is handled by a separate job
		stage('Archive') {
			steps {
				unstash 'dist-aseba-debian'
				unstash 'dist-aseba-macos'
				unstash 'dist-aseba-windows'
				archiveArtifacts artifacts: 'dist/**', fingerprint: true, onlyIfSuccessful: true
			}
		}
	}
}
